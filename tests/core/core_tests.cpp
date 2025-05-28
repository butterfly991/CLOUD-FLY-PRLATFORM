#include <gtest/gtest.h>
#include "core/ParentCore.h"
#include "core/StorageManager.h"
#include "core/NetworkManager.h"
#include "core/LoadBalancer.h"
#include "core/blockchain/MultiCoreBlockchain.h"
#include "core/drivers/compute_ops.h"
#include "core/drivers/memory_ops.h"
#include "core/drivers/blockchain_ops.h"
#include "core/drivers/network_ops.h"
#include "core/drivers/atomic_ops.h"
#include "core/drivers/thread_ops.h"
#include "core/drivers/math_ops.h"
#include "core/drivers/buffer_ops.h"
#include "core/drivers/cpu_info.h"
#include "core/drivers/fast_memcpy.h"

using namespace core;

class CoreSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize core components
        parent_core = std::make_unique<ParentCore>();
        storage_manager = std::make_unique<StorageManager>();
        network_manager = std::make_unique<NetworkManager>();
        load_balancer = std::make_unique<LoadBalancer>();
        blockchain = std::make_unique<MultiCoreBlockchain>();
    }

    void TearDown() override {
        // Cleanup
        parent_core.reset();
        storage_manager.reset();
        network_manager.reset();
        load_balancer.reset();
        blockchain.reset();
    }

    std::unique_ptr<ParentCore> parent_core;
    std::unique_ptr<StorageManager> storage_manager;
    std::unique_ptr<NetworkManager> network_manager;
    std::unique_ptr<LoadBalancer> load_balancer;
    std::unique_ptr<MultiCoreBlockchain> blockchain;
};

// Test core initialization
TEST_F(CoreSystemTest, CoreInitialization) {
    EXPECT_TRUE(parent_core->initialize());
    EXPECT_TRUE(storage_manager->start());
    EXPECT_TRUE(network_manager->start());
    EXPECT_TRUE(load_balancer->start());
    EXPECT_TRUE(blockchain->initialize());
}

// Test core operations
TEST_F(CoreSystemTest, CoreOperations) {
    // Test parent core operations
    EXPECT_TRUE(parent_core->start());
    EXPECT_TRUE(parent_core->is_running());
    
    parent_core->pause();
    EXPECT_TRUE(parent_core->is_paused());
    
    parent_core->resume();
    EXPECT_FALSE(parent_core->is_paused());
    
    parent_core->stop();
    EXPECT_FALSE(parent_core->is_running());
}

// Test storage operations
TEST_F(CoreSystemTest, StorageOperations) {
    StorageManager::StorageConfig config;
    config.path = "/tmp/test";
    config.block_size = 4096;
    config.cache_size = 1024 * 1024;
    config.max_files = 100;
    config.timeout = 5000;

    size_t file_id = storage_manager->open_file("test.txt", config);
    EXPECT_GT(file_id, 0);

    const char* test_data = "Hello, World!";
    size_t written = storage_manager->write_file(file_id, test_data, strlen(test_data), 0);
    EXPECT_EQ(written, strlen(test_data));

    char buffer[256] = {0};
    size_t read = storage_manager->read_file(file_id, buffer, sizeof(buffer), 0);
    EXPECT_EQ(read, strlen(test_data));
    EXPECT_STREQ(buffer, test_data);

    storage_manager->close_file(file_id);
}

// Test network operations
TEST_F(CoreSystemTest, NetworkOperations) {
    NetworkManager::ConnectionConfig config;
    config.host = "localhost";
    config.port = 8080;
    config.buffer_size = 4096;
    config.max_connections = 100;
    config.timeout = 5000;

    size_t conn_id = network_manager->connect(config);
    EXPECT_GT(conn_id, 0);

    const char* test_data = "Hello, Network!";
    size_t sent = network_manager->send(conn_id, test_data, strlen(test_data));
    EXPECT_EQ(sent, strlen(test_data));

    char buffer[256] = {0};
    size_t received = network_manager->receive(conn_id, buffer, sizeof(buffer));
    EXPECT_GT(received, 0);

    network_manager->disconnect(conn_id);
}

// Test load balancer
TEST_F(CoreSystemTest, LoadBalancing) {
    Task task;
    task.type = TaskType::COMPUTE;
    task.priority = TaskPriority::HIGH;
    task.data = "Test task";

    size_t task_id = load_balancer->submit_task(task);
    EXPECT_GT(task_id, 0);

    TaskStatus status = load_balancer->get_task_status(task_id);
    EXPECT_NE(status, TaskStatus::UNKNOWN);

    load_balancer->cancel_task(task_id);
    status = load_balancer->get_task_status(task_id);
    EXPECT_EQ(status, TaskStatus::CANCELLED);
}

// Test blockchain operations
TEST_F(CoreSystemTest, BlockchainOperations) {
    Transaction tx;
    tx.type = TransactionType::TRANSFER;
    tx.data = "Test transaction";

    size_t tx_id = blockchain->submit_transaction(tx);
    EXPECT_GT(tx_id, 0);

    TransactionStatus status = blockchain->get_transaction_status(tx_id);
    EXPECT_NE(status, TransactionStatus::UNKNOWN);

    blockchain->cancel_transaction(tx_id);
    status = blockchain->get_transaction_status(tx_id);
    EXPECT_EQ(status, TransactionStatus::CANCELLED);
}

// Test driver operations
TEST_F(CoreSystemTest, DriverOperations) {
    // Test compute operations
    int result = compute_ops_add(5, 3);
    EXPECT_EQ(result, 8);

    // Test memory operations
    void* ptr = memory_ops_allocate(1024);
    EXPECT_NE(ptr, nullptr);
    memory_ops_free(ptr);

    // Test atomic operations
    atomic_int value = 0;
    atomic_ops_increment(&value);
    EXPECT_EQ(value, 1);

    // Test thread operations
    thread_ops_yield();
    EXPECT_TRUE(thread_ops_is_running());

    // Test math operations
    double result_math = math_ops_sqrt(16.0);
    EXPECT_DOUBLE_EQ(result_math, 4.0);

    // Test buffer operations
    char buffer[256];
    buffer_ops_clear(buffer, sizeof(buffer));
    EXPECT_EQ(buffer[0], 0);

    // Test CPU info
    EXPECT_GT(cpu_info_get_cores(), 0);

    // Test fast memcpy
    char src[256] = "Test data";
    char dst[256] = {0};
    fast_memcpy(dst, src, strlen(src));
    EXPECT_STREQ(dst, src);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 