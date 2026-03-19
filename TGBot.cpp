#include "TGBot.hpp"
#include <curl/curl.h>
#include <iostream>
#include <chrono>

struct CurlMemoryStruct {
    char* memory;
    size_t size;
};

static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    struct CurlMemoryStruct* mem = (struct CurlMemoryStruct*)userp;
    
    char* ptr = (char*)realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        return 0;
    }
    
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    
    return realsize;
}

TGBot::TGBot(const std::string& token, const std::string& chatId)
    : m_token(token), m_chatId(chatId), m_running(false), m_offset(0) {
    curl_global_init(CURL_GLOBAL_ALL);
}

TGBot::~TGBot() {
    stop();
    curl_global_cleanup();
}

void TGBot::start() {
    if (m_running) return;
    if (m_token.empty()) return;

    m_running = true;
    m_thread = std::thread(&TGBot::pollLoop, this);
    
    // Announce startup
    if (!m_chatId.empty()) {
        sendMenu(std::stoll(m_chatId), "🤖 *Trongo C++ 核心引擎已上线！*\n双线程驱动，原生控制面板已就绪。");
    }
}

void TGBot::stop() {
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

nlohmann::json TGBot::requestAPI(const std::string& method, const nlohmann::json& payload) {
    CURL* curl = curl_easy_init();
    if (!curl) return nlohmann::json();

    std::string url = "https://api.telegram.org/bot" + m_token + "/" + method;
    std::string dataStr = payload.is_null() ? "" : payload.dump();

    struct CurlMemoryStruct chunk;
    chunk.memory = (char*)malloc(1);
    chunk.size = 0;

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    if (!dataStr.empty()) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, dataStr.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl);
    nlohmann::json responseConfig;

    if (res == CURLE_OK) {
        try {
            responseConfig = nlohmann::json::parse(chunk.memory);
        } catch (...) {}
    }

    free(chunk.memory);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return responseConfig;
}

void TGBot::sendMessage(long long chatId, const std::string& text) {
    nlohmann::json payload = {
        {"chat_id", chatId},
        {"text", text},
        {"parse_mode", "Markdown"}
    };
    requestAPI("sendMessage", payload);
}

void TGBot::sendMenu(long long chatId, const std::string& title) {
    nlohmann::json payload = {
        {"chat_id", chatId},
        {"text", title},
        {"parse_mode", "Markdown"},
        {"reply_markup", {
            {"inline_keyboard", nlohmann::json::array({
                nlohmann::json::array({
                    {{"text", "🚀 启动挂机"}, {"callback_data", "cmd_start"}},
                    {{"text", "🛑 紧急停止"}, {"callback_data", "cmd_stop"}}
                }),
                nlohmann::json::array({
                    {{"text", "⚡ 查算力"}, {"callback_data", "cmd_speed"}},
                    {{"text", "🏆 查结果"}, {"callback_data", "cmd_result"}}
                })
            })}
        }}
    };
    requestAPI("sendMessage", payload);
}

void TGBot::answerCallback(const std::string& callbackQueryId) {
    nlohmann::json payload = {
        {"callback_query_id", callbackQueryId}
    };
    requestAPI("answerCallbackQuery", payload);
}

void TGBot::pollLoop() {
    while (m_running) {
        nlohmann::json payload = {
            {"offset", m_offset},
            {"timeout", 20}
        };
        nlohmann::json res = requestAPI("getUpdates", payload);

        if (res.is_object() && res.contains("result") && res["result"].is_array()) {
            for (auto& upd : res["result"]) {
                long long updateId = upd.value("update_id", 0LL);
                m_offset = updateId + 1;

                if (upd.contains("callback_query")) {
                    auto& cb = upd["callback_query"];
                    std::string cbId = cb.value("id", "");
                    std::string data = cb.value("data", "");
                    long long chatId = cb["message"]["chat"].value("id", 0LL);
                    
                    answerCallback(cbId);

                    if (m_cbHandler) {
                        m_cbHandler(data, cbId, chatId);
                    }
                } else if (upd.contains("message") && upd["message"].contains("text")) {
                    std::string text = upd["message"].value("text", "");
                    long long chatId = upd["message"]["chat"].value("id", 0LL);
                    
                    if (m_cmdHandler) {
                        m_cmdHandler(text, chatId);
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}
