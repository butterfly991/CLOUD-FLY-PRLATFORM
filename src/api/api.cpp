#include "api/api.h"
#include <iostream>
#include <set>

namespace api {

class CoreAPIImpl : public CoreAPI {
public:
    CoreAPIImpl() = default;
    ~CoreAPIImpl() override = default;
    bool initialize() override {
        std::cout << "[API] Инициализация ядра API\n";
        return true;
    }
    bool registerService(const std::string& name) override {
        std::cout << "[API] Регистрация сервиса: " << name << "\n";
        services_.insert(name);
        return true;
    }
    Response handleRequest(const Request& req) override {
        std::cout << "[API] Обработка запроса: " << req.endpoint << "\n";
        Response resp;
        resp.status = 200;
        resp.body = "OK";
        return resp;
    }
    std::string getStatus() const override {
        return "API is running";
    }
    void logEvent(const std::string& event) override {
        std::cout << "[API] Лог: " << event << "\n";
    }
    bool addUser(const std::string& user, const std::string& role) override {
        users_.insert(user + ":" + role);
        return true;
    }
    bool removeUser(const std::string& user) override {
        for (auto it = users_.begin(); it != users_.end(); ) {
            if (it->find(user + ":") == 0) {
                it = users_.erase(it);
            } else {
                ++it;
            }
        }
        return true;
    }
    std::vector<std::string> listUsers() const override {
        std::vector<std::string> result;
        for (const auto& u : users_) result.push_back(u);
        return result;
    }
private:
    std::set<std::string> services_;
    std::set<std::string> users_;
};

std::shared_ptr<CoreAPI> createCoreAPI() {
    return std::make_shared<CoreAPIImpl>();
}

} // namespace api 