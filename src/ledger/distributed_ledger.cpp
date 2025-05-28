#include "distributed_ledger.h"
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace cloud {

DistributedLedger::DistributedLedger(
    const std::string& config_path,
    std::shared_ptr<Logger> logger,
    std::shared_ptr<MetricsReporter> metrics
) : m_logger(logger), m_metrics(metrics) {
    if (!m_logger || !m_metrics) {
        throw std::invalid_argument("Invalid logger or metrics reporter");
    }
    
    try {
        load_configuration(config_path);
        initialize_consensus();
        setup_network();
        start_consensus();
        
        m_logger->log(Logger::Level::Info, "DistributedLedger initialized", {
            {"config_path", config_path},
            {"consensus_type", m_config.consensus_type},
            {"network_mode", m_config.network_mode}
        });
    } catch (const std::exception& ex) {
        m_logger->log(Logger::Level::Critical, "Failed to initialize DistributedLedger", {
            {"error", ex.what()}
        });
        throw;
    }
}

DistributedLedger::~DistributedLedger() {
    try {
        stop_consensus();
        cleanup_resources();
        
        m_logger->log(Logger::Level::Info, "DistributedLedger shutdown completed");
    } catch (const std::exception& ex) {
        m_logger->log(Logger::Level::Error, "Error during DistributedLedger shutdown", {
            {"error", ex.what()}
        });
    }
}

void DistributedLedger::load_configuration(const std::string& config_path) {
    try {
        // Загрузка конфигурации из файла
        ConfigLoader loader;
        m_config = loader.load_ledger_config(config_path);
        
        // Валидация конфигурации
        if (!validate_config(m_config)) {
            throw std::runtime_error("Invalid ledger configuration");
        }
        
        m_logger->log(Logger::Level::Info, "Ledger configuration loaded", {
            {"config_path", config_path}
        });
    } catch (const std::exception& ex) {
        m_logger->log(Logger::Level::Error, "Failed to load ledger configuration", {
            {"error", ex.what()}
        });
        throw;
    }
}

void DistributedLedger::initialize_consensus() {
    try {
        // Инициализация консенсусного механизма
        switch (m_config.consensus_type) {
            case ConsensusType::PBFT:
                m_consensus = std::make_unique<PBFTEngine>(m_config, m_logger);
                break;
            case ConsensusType::RAFT:
                m_consensus = std::make_unique<RaftEngine>(m_config, m_logger);
                break;
            default:
                throw std::runtime_error("Unsupported consensus type");
        }
        
        // Инициализация криптографических компонентов
        initialize_crypto();
        
        m_logger->log(Logger::Level::Info, "Consensus engine initialized", {
            {"type", static_cast<int>(m_config.consensus_type)}
        });
    } catch (const std::exception& ex) {
        m_logger->log(Logger::Level::Error, "Failed to initialize consensus", {
            {"error", ex.what()}
        });
        throw;
    }
}

void DistributedLedger::setup_network() {
    try {
        // Настройка сетевого стека
        m_network = std::make_unique<NetworkStack>(m_config.network_config);
        
        // Регистрация обработчиков сообщений
        m_network->register_handler(MessageType::CONSENSUS, 
            [this](const Message& msg) { handle_consensus_message(msg); });
        m_network->register_handler(MessageType::BLOCK, 
            [this](const Message& msg) { handle_block_message(msg); });
        m_network->register_handler(MessageType::TRANSACTION, 
            [this](const Message& msg) { handle_transaction_message(msg); });
        
        m_logger->log(Logger::Level::Info, "Network stack configured", {
            {"mode", m_config.network_mode}
        });
    } catch (const std::exception& ex) {
        m_logger->log(Logger::Level::Error, "Failed to setup network", {
            {"error", ex.what()}
        });
        throw;
    }
}

void DistributedLedger::start_consensus() {
    try {
        if (!m_consensus) {
            throw std::runtime_error("Consensus engine not initialized");
        }
        
        // Запуск консенсусного механизма
        m_consensus->start();
        
        // Запуск сетевого стека
        m_network->start();
        
        // Запуск обработки транзакций
        start_transaction_processor();
        
        m_logger->log(Logger::Level::Info, "Consensus engine started");
    } catch (const std::exception& ex) {
        m_logger->log(Logger::Level::Error, "Failed to start consensus", {
            {"error", ex.what()}
        });
        throw;
    }
}

void DistributedLedger::stop_consensus() {
    try {
        // Остановка обработки транзакций
        stop_transaction_processor();
        
        // Остановка сетевого стека
        if (m_network) {
            m_network->stop();
        }
        
        // Остановка консенсусного механизма
        if (m_consensus) {
            m_consensus->stop();
        }
        
        m_logger->log(Logger::Level::Info, "Consensus engine stopped");
    } catch (const std::exception& ex) {
        m_logger->log(Logger::Level::Error, "Error during consensus shutdown", {
            {"error", ex.what()}
        });
    }
}

void DistributedLedger::cleanup_resources() {
    try {
        // Очистка ресурсов
        m_consensus.reset();
        m_network.reset();
        m_crypto.reset();
        
        m_logger->log(Logger::Level::Info, "Resources cleaned up");
    } catch (const std::exception& ex) {
        m_logger->log(Logger::Level::Error, "Error during resource cleanup", {
            {"error", ex.what()}
        });
    }
}

void DistributedLedger::initialize_crypto() {
    try {
        // Инициализация криптографических компонентов
        m_crypto = std::make_unique<CryptoEngine>(m_config.crypto_config);
        
        // Генерация ключей
        m_crypto->generate_keys();
        
        m_logger->log(Logger::Level::Info, "Crypto engine initialized");
    } catch (const std::exception& ex) {
        m_logger->log(Logger::Level::Error, "Failed to initialize crypto engine", {
            {"error", ex.what()}
        });
        throw;
    }
}

void DistributedLedger::start_transaction_processor() {
    try {
        // Запуск обработчика транзакций
        m_tx_processor = std::make_unique<TransactionProcessor>(
            m_config.tx_config,
            m_logger,
            m_metrics
        );
        
        m_tx_processor->start();
        
        m_logger->log(Logger::Level::Info, "Transaction processor started");
    } catch (const std::exception& ex) {
        m_logger->log(Logger::Level::Error, "Failed to start transaction processor", {
            {"error", ex.what()}
        });
        throw;
    }
}

void DistributedLedger::stop_transaction_processor() {
    try {
        if (m_tx_processor) {
            m_tx_processor->stop();
            m_tx_processor.reset();
        }
        
        m_logger->log(Logger::Level::Info, "Transaction processor stopped");
    } catch (const std::exception& ex) {
        m_logger->log(Logger::Level::Error, "Error during transaction processor shutdown", {
            {"error", ex.what()}
        });
    }
}

void DistributedLedger::handle_consensus_message(const Message& msg) {
    try {
        if (!m_consensus) {
            throw std::runtime_error("Consensus engine not initialized");
        }
        
        // Обработка консенсусного сообщения
        m_consensus->handle_message(msg);
        
        m_metrics->record_message_processed(MessageType::CONSENSUS);
    } catch (const std::exception& ex) {
        m_logger->log(Logger::Level::Error, "Failed to handle consensus message", {
            {"error", ex.what()}
        });
    }
}

void DistributedLedger::handle_block_message(const Message& msg) {
    try {
        // Обработка сообщения о блоке
        Block block = deserialize_block(msg.data);
        
        // Проверка подписи блока
        if (!verify_block_signature(block)) {
            throw std::runtime_error("Invalid block signature");
        }
        
        // Добавление блока в цепочку
        add_block(block);
        
        m_metrics->record_message_processed(MessageType::BLOCK);
    } catch (const std::exception& ex) {
        m_logger->log(Logger::Level::Error, "Failed to handle block message", {
            {"error", ex.what()}
        });
    }
}

void DistributedLedger::handle_transaction_message(const Message& msg) {
    try {
        // Обработка транзакции
        Transaction tx = deserialize_transaction(msg.data);
        
        // Проверка подписи транзакции
        if (!verify_transaction_signature(tx)) {
            throw std::runtime_error("Invalid transaction signature");
        }
        
        // Добавление транзакции в пул
        add_transaction(tx);
        
        m_metrics->record_message_processed(MessageType::TRANSACTION);
    } catch (const std::exception& ex) {
        m_logger->log(Logger::Level::Error, "Failed to handle transaction message", {
            {"error", ex.what()}
        });
    }
}

bool DistributedLedger::verify_block_signature(const Block& block) {
    try {
        if (!m_crypto) {
            throw std::runtime_error("Crypto engine not initialized");
        }
        
        // Проверка подписи блока
        return m_crypto->verify_signature(
            block.signature,
            block.hash,
            block.miner_public_key
        );
    } catch (const std::exception& ex) {
        m_logger->log(Logger::Level::Error, "Failed to verify block signature", {
            {"error", ex.what()}
        });
        return false;
    }
}

bool DistributedLedger::verify_transaction_signature(const Transaction& tx) {
    try {
        if (!m_crypto) {
            throw std::runtime_error("Crypto engine not initialized");
        }
        
        // Проверка подписи транзакции
        return m_crypto->verify_signature(
            tx.signature,
            tx.hash,
            tx.sender_public_key
        );
    } catch (const std::exception& ex) {
        m_logger->log(Logger::Level::Error, "Failed to verify transaction signature", {
            {"error", ex.what()}
        });
        return false;
    }
}

void DistributedLedger::add_block(const Block& block) {
    try {
        // Добавление блока в цепочку
        m_chain.add_block(block);
        
        // Обновление состояния
        update_state(block);
        
        // Очистка обработанных транзакций
        remove_processed_transactions(block);
        
        m_metrics->record_block_added(block.height);
    } catch (const std::exception& ex) {
        m_logger->log(Logger::Level::Error, "Failed to add block", {
            {"error", ex.what()},
            {"block_height", block.height}
        });
        throw;
    }
}

void DistributedLedger::add_transaction(const Transaction& tx) {
    try {
        // Добавление транзакции в пул
        m_tx_pool.add_transaction(tx);
        
        m_metrics->record_transaction_added();
    } catch (const std::exception& ex) {
        m_logger->log(Logger::Level::Error, "Failed to add transaction", {
            {"error", ex.what()}
        });
        throw;
    }
}

void DistributedLedger::update_state(const Block& block) {
    try {
        // Обновление состояния на основе блока
        for (const auto& tx : block.transactions) {
            apply_transaction(tx);
        }
        
        m_metrics->record_state_updated();
    } catch (const std::exception& ex) {
        m_logger->log(Logger::Level::Error, "Failed to update state", {
            {"error", ex.what()}
        });
        throw;
    }
}

void DistributedLedger::apply_transaction(const Transaction& tx) {
    try {
        // Применение транзакции к состоянию
        m_state.apply_transaction(tx);
        
        m_metrics->record_transaction_applied();
    } catch (const std::exception& ex) {
        m_logger->log(Logger::Level::Error, "Failed to apply transaction", {
            {"error", ex.what()}
        });
        throw;
    }
}

void DistributedLedger::remove_processed_transactions(const Block& block) {
    try {
        // Удаление обработанных транзакций из пула
        for (const auto& tx : block.transactions) {
            m_tx_pool.remove_transaction(tx.hash);
        }
        
        m_metrics->record_transactions_removed(block.transactions.size());
    } catch (const std::exception& ex) {
        m_logger->log(Logger::Level::Error, "Failed to remove processed transactions", {
            {"error", ex.what()}
        });
        throw;
    }
}

} // namespace cloud 