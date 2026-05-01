#include "ResultStore.hpp"

ResultStore::ResultStore() : m_db(nullptr) {}

ResultStore::~ResultStore() { close(); }

bool ResultStore::exec(const std::string& sql) {
    char* err = nullptr;
    int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        m_lastError = err ? err : "sqlite3_exec failed";
        sqlite3_free(err);
        return false;
    }
    return true;
}

bool ResultStore::open(const std::string& path, const std::string& key) {
    if (m_db) close();
    if (sqlite3_open(path.c_str(), &m_db) != SQLITE_OK) {
        m_lastError = m_db ? sqlite3_errmsg(m_db) : "sqlite3_open failed";
        if (m_db) { sqlite3_close(m_db); m_db = nullptr; }
        return false;
    }

    if (!key.empty()) {
        // Escape single quotes for the PRAGMA key string literal.
        std::string esc;
        for (char c : key) {
            if (c == '\'') esc += "''";
            else esc += c;
        }
        std::string pragma = "PRAGMA key = '" + esc + "';";
        if (!exec(pragma)) { close(); return false; }
    }

    // First read forces SQLCipher to verify the key against existing DB.
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, "SELECT count(*) FROM sqlite_master;", -1, &stmt, nullptr) != SQLITE_OK
        || sqlite3_step(stmt) != SQLITE_ROW) {
        m_lastError = sqlite3_errmsg(m_db);
        if (stmt) sqlite3_finalize(stmt);
        close();
        return false;
    }
    sqlite3_finalize(stmt);

    if (!exec(
        "CREATE TABLE IF NOT EXISTS results ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  private_key TEXT NOT NULL,"
        "  address TEXT NOT NULL,"
        "  rule_pattern TEXT,"
        "  prefix_count INTEGER,"
        "  suffix_count INTEGER,"
        "  elapsed_seconds INTEGER,"
        "  created_at INTEGER NOT NULL DEFAULT (strftime('%s','now'))"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_results_id_desc ON results(id DESC);")) {
        close();
        return false;
    }
    return true;
}

void ResultStore::close() {
    if (m_db) { sqlite3_close(m_db); m_db = nullptr; }
}

bool ResultStore::insert(const std::string& privateKey,
                         const std::string& address,
                         const std::string& rulePattern,
                         int prefixCount, int suffixCount,
                         int elapsedSeconds) {
    if (!m_db) return false;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO results "
                      "(private_key, address, rule_pattern, prefix_count, suffix_count, elapsed_seconds) "
                      "VALUES (?, ?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    sqlite3_bind_text(stmt, 1, privateKey.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, address.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, rulePattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, prefixCount);
    sqlite3_bind_int(stmt, 5, suffixCount);
    sqlite3_bind_int(stmt, 6, elapsedSeconds);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!ok) m_lastError = sqlite3_errmsg(m_db);
    sqlite3_finalize(stmt);
    return ok;
}

static void bindRow(sqlite3_stmt* stmt, ResultEntry& e) {
    e.id              = sqlite3_column_int64(stmt, 0);
    e.privateKey      = (const char*)sqlite3_column_text(stmt, 1);
    e.address         = (const char*)sqlite3_column_text(stmt, 2);
    const unsigned char* rp = sqlite3_column_text(stmt, 3);
    e.rulePattern     = rp ? (const char*)rp : "";
    e.prefixCount     = sqlite3_column_int(stmt, 4);
    e.suffixCount     = sqlite3_column_int(stmt, 5);
    e.elapsedSeconds  = sqlite3_column_int(stmt, 6);
    e.createdAt       = sqlite3_column_int64(stmt, 7);
}

std::vector<ResultEntry> ResultStore::recent(int limit) {
    std::vector<ResultEntry> out;
    if (!m_db) return out;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, private_key, address, rule_pattern, "
                      "       prefix_count, suffix_count, elapsed_seconds, created_at "
                      "FROM results ORDER BY id DESC LIMIT ?;";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return out;
    }
    sqlite3_bind_int(stmt, 1, limit);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ResultEntry e;
        bindRow(stmt, e);
        out.push_back(e);
    }
    sqlite3_finalize(stmt);
    return out;
}

bool ResultStore::getById(long long id, ResultEntry& out) {
    if (!m_db) return false;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, private_key, address, rule_pattern, "
                      "       prefix_count, suffix_count, elapsed_seconds, created_at "
                      "FROM results WHERE id = ?;";
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        return false;
    }
    sqlite3_bind_int64(stmt, 1, id);
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        bindRow(stmt, out);
        found = true;
    }
    sqlite3_finalize(stmt);
    return found;
}

int ResultStore::count() {
    if (!m_db) return 0;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, "SELECT count(*) FROM results;", -1, &stmt, nullptr) != SQLITE_OK)
        return 0;
    int n = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) n = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return n;
}
