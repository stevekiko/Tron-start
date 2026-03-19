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
    
    // Announce startup and show persistent keyboard
    if (!m_chatId.empty()) {
        sendPersistentKeyboard(std::stoll(m_chatId));
        sendMenu(std::stoll(m_chatId), "🤖 *Tron-start C++ 核心引擎已上线！*\n双线程驱动，原生控制面板已就绪。");
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
                    {{"text", "🎯 设置规则"}, {"callback_data", "cmd_rule"}}
                }),
                nlohmann::json::array({
                    {{"text", "⚡ 查算力"}, {"callback_data", "cmd_speed"}},
                    {{"text", "🏆 查结果"}, {"callback_data", "cmd_result"}}
                }),
                nlohmann::json::array({
                    {{"text", "🛑 紧急停止"}, {"callback_data", "cmd_stop"}}
                })
            })}
        }}
    };
    requestAPI("sendMessage", payload);
}

void TGBot::sendRuleMenu(long long chatId) {
    nlohmann::json payload = {
        {"chat_id", chatId},
        {"text", "🎯 *请选择你想要的爆号号段/规则类型:*\n_选中后，程序会自动将底层规则库 profanity.txt 替换为你选中的豹子号格式!_\n"},
        {"parse_mode", "Markdown"},
        {"reply_markup", {
            {"inline_keyboard", nlohmann::json::array({
                nlohmann::json::array({
                    {{"text", "🔢 纯数字连号 (1-9连击)"}, {"callback_data", "set_rule_num"}}
                }),
                nlohmann::json::array({
                    {{"text", "🔠 纯字母连号 (A-Z连击)"}, {"callback_data", "set_rule_alpha"}}
                }),
                nlohmann::json::array({
                    {{"text", "👑 数字+字母 全开打尽"}, {"callback_data", "set_rule_all"}}
                })
            })}
        }}
    };
    requestAPI("sendMessage", payload);
}

void TGBot::sendStartMenu(long long chatId) {
    nlohmann::json payload = {
        {"chat_id", chatId},
        {"text", "🎯 *请选择本次任务的碰撞难度 (以默认规则 profanity.txt 为基准):*\n\n_也支持直接在聊天框发送自定义完整地址串开始碰撞_"},
        {"parse_mode", "Markdown"},
        {"reply_markup", {
            {"inline_keyboard", nlohmann::json::array({
                nlohmann::json::array({
                    {{"text", "🎲 匹配 6 位极品后缀 (极快)"}, {"callback_data", "run_0_6"}}
                }),
                nlohmann::json::array({
                    {{"text", "🎲 匹配 7 位极品后缀 (较快)"}, {"callback_data", "run_0_7"}}
                }),
                nlohmann::json::array({
                    {{"text", "💎 匹配 8 位极品后缀 (耗时)"}, {"callback_data", "run_0_8"}}
                }),
                nlohmann::json::array({
                    {{"text", "🔥 前缀1位 + 后缀8位 (地狱爆率)"}, {"callback_data", "run_1_8"}}
                })
            })}
        }}
    };
    requestAPI("sendMessage", payload);
}

void TGBot::sendPersistentKeyboard(long long chatId) {
    nlohmann::json payload = {
        {"chat_id", chatId},
        {"text", "📢 *快捷键盘已激活！*\n点击下方按钟即可操作，无需手动输入命令。"},
        {"parse_mode", "Markdown"},
        {"reply_markup", {
            {"keyboard", nlohmann::json::array({
                nlohmann::json::array({
                    {{"text", "🚀 启动挂机"}},
                    {{"text", "🎯 设置规则"}}
                }),
                nlohmann::json::array({
                    {{"text", "⚡ 查算力"}},
                    {{"text", "🏆 查结果"}}
                }),
                nlohmann::json::array({
                    {{"text", "🔴 紧急停止"}},
                    {{"text", "📋 主菜单"}}
                })
            })},
            {"resize_keyboard", true},
            {"persistent", true}
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
        try {
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
        } catch (std::exception &e) {
            std::cout << "Polling Loop Exception: " << e.what() << std::endl;
        } catch (...) {
            std::cout << "Polling Loop Unknown Exception" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}
