#pragma once
#include <string>
#include <vector>
#include <sqlcipher/sqlite3.h>

struct ResultEntry {
    long long id;
    std::string privateKey;
    std::string address;
    std::string rulePattern;
    int prefixCount;
    int suffixCount;
    int elapsedSeconds;
    long long createdAt;
};

class ResultStore {
public:
    ResultStore();
    ~ResultStore();

    // Open SQLCipher DB. Empty key => plain SQLite.
    // Creates schema on first use. Returns false on key mismatch / IO error.
    bool open(const std::string& path, const std::string& key);
    void close();
    bool isOpen() const { return m_db != nullptr; }

    bool insert(const std::string& privateKey,
                const std::string& address,
                const std::string& rulePattern,
                int prefixCount, int suffixCount,
                int elapsedSeconds);

    // Newest first.
    std::vector<ResultEntry> recent(int limit);
    bool getById(long long id, ResultEntry& out);
    int count();

    // === config(key, value) — TG_TOKEN, TG_CHAT_ID, etc. ===
    std::string getConfig(const std::string& key);
    bool setConfig(const std::string& key, const std::string& value);

    // === rules(pattern, ord) — TRON vanity matching patterns ===
    std::vector<std::string> getRules();
    int rulesCount();
    // Atomically replace the entire rules table with the given patterns (DELETE + INSERT).
    bool replaceRules(const std::vector<std::string>& patterns);

    const std::string& lastError() const { return m_lastError; }

private:
    sqlite3* m_db;
    std::string m_lastError;
    bool exec(const std::string& sql);
};
