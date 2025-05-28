#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace api {

// Структура для описания запроса
struct Request {
    std::string endpoint;
    std::unordered_map<std::string, std::string> params;
    std::string body;
};

// Структура для ответа
struct Response {
    int status;
    std::string body;
};

// Базовый интерфейс API
class CoreAPI {
public:
    virtual ~CoreAPI() = default;
    // Инициализация API
    virtual bool initialize() = 0;
    // Регистрация сервиса
    virtual bool registerService(const std::string& name) = 0;
    // Обработка запроса
    virtual Response handleRequest(const Request& req) = 0;
    // Получить статус
    virtual std::string getStatus() const = 0;
    // Логирование события
    virtual void logEvent(const std::string& event) = 0;
    // Управление пользователями
    virtual bool addUser(const std::string& user, const std::string& role) = 0;
    virtual bool removeUser(const std::string& user) = 0;
    virtual std::vector<std::string> listUsers() const = 0;
};

// Фабрика для создания API
std::shared_ptr<CoreAPI> createCoreAPI();

} // namespace api 