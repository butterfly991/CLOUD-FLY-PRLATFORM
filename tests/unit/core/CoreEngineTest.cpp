#include <gtest/gtest.h>
#include <core/CoreEngine.h>
#include <monitoring/metrics_reporter.h>
#include <security/secret_vault.h>

using namespace testing;

class CoreEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        metrics = std::make_shared<monitoring::MetricsReporter>();
        vault = std::make_shared<security::SecretVault>();
        engine = std::make_unique<core::CoreEngine>(
            "config/test/config.json",
            vault,
            metrics
        );
    }

    void TearDown() override {
        engine->graceful_shutdown();
    }

    std::shared_ptr<monitoring::MetricsReporter> metrics;
    std::shared_ptr<security::SecretVault> vault;
    std::unique_ptr<core::CoreEngine> engine;
};

TEST_F(CoreEngineTest, Initialization) {
    EXPECT_NO_THROW(engine->start());
    EXPECT_TRUE(engine->is_running());
}

TEST_F(CoreEngineTest, NetworkManager) {
    engine->start();
    auto& network = engine->get_network_manager();
    EXPECT_TRUE(network.is_initialized());
}

TEST_F(CoreEngineTest, StorageManager) {
    engine->start();
    auto& storage = engine->get_storage_manager();
    
    storage.put("test_key", "test_value");
    auto value = storage.get("test_key");
    EXPECT_EQ(value, "test_value");
}

TEST_F(CoreEngineTest, DistributedLedger) {
    engine->start();
    auto& ledger = engine->get_distributed_ledger();
    
    EXPECT_NO_THROW(ledger.submit_transaction({
        "type": "test",
        "data": "Hello, Blockchain!"
    }));
}

TEST_F(CoreEngineTest, GracefulShutdown) {
    engine->start();
    EXPECT_NO_THROW(engine->graceful_shutdown());
    EXPECT_FALSE(engine->is_running());
}

TEST_F(CoreEngineTest, EmergencyStop) {
    engine->start();
    EXPECT_NO_THROW(engine->emergency_stop());
    EXPECT_FALSE(engine->is_running());
} 