#ifndef TGBOT_HPP
#define TGBOT_HPP

#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include "json.hpp"

class TGBot {
public:
    using CallbackHandler = std::function<void(const std::string&, const std::string&, long long)>;
    using CommandHandler = std::function<void(const std::string&, long long)>;

    TGBot(const std::string& token, const std::string& chatId);
    ~TGBot();

    void start();
    void stop();

    void sendMessage(long long chatId, const std::string& text);
    void sendMenu(long long chatId, const std::string& title = "💻 *Trongo C++ 原生控制面板*");
    void sendStartMenu(long long chatId);
    void answerCallback(const std::string& callbackQueryId);

    void setCallbackHandler(CallbackHandler handler) { m_cbHandler = handler; }
    void setCommandHandler(CommandHandler handler) { m_cmdHandler = handler; }

private:
    void pollLoop();
    nlohmann::json requestAPI(const std::string& method, const nlohmann::json& payload);

    std::string m_token;
    std::string m_chatId;
    std::atomic<bool> m_running;
    std::thread m_thread;
    long long m_offset;

    CallbackHandler m_cbHandler;
    CommandHandler m_cmdHandler;
};

#endif // TGBOT_HPP
