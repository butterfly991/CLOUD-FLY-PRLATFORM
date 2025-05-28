#include <core/CoreEngine.h>
#include <blockchain/distributed_ledger.h>
#include <network/network_manager.h>
#include <storage/storage_manager.h>
#include <monitoring/metrics_reporter.h>
#include <security/secret_vault.h>

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

int main(int argc, char* argv[]) {
    try {
        // Инициализация компонентов
        auto metrics = std::make_shared<monitoring::MetricsReporter>();
        auto vault = std::make_shared<security::SecretVault>();
        
        // Создание и запуск движка
        auto engine = std::make_unique<core::CoreEngine>(
            "config/dev/config.json",
            vault,
            metrics
        );
        
        std::cout << "Starting Cloud Service Platform..." << std::endl;
        engine->start();
        
        // Получение доступа к компонентам
        auto& network = engine->get_network_manager();
        auto& storage = engine->get_storage_manager();
        auto& ledger = engine->get_distributed_ledger();
        
        // Пример работы с сетью
        network.register_handler("test", [](const auto& msg) {
            std::cout << "Received test message: " << msg << std::endl;
        });
        
        // Пример работы с хранилищем
        storage.put("test_key", "test_value");
        auto value = storage.get("test_key");
        std::cout << "Retrieved value: " << value << std::endl;
        
        // Пример работы с блокчейном
        ledger.submit_transaction({
            "type": "test",
            "data": "Hello, Blockchain!"
        });
        
        // Работаем некоторое время
        std::this_thread::sleep_for(5s);
        
        // Graceful shutdown
        std::cout << "Shutting down..." << std::endl;
        engine->graceful_shutdown();
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
} 