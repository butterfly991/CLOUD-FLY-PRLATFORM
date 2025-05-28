#include <gtest/gtest.h>
#include "core/NetworkManager.h"
#include "core/drivers/network_ops.h"
#include "core/drivers/thread_ops.h"

using namespace core;

class NetworkManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<NetworkManager>();
        EXPECT_TRUE(manager->start());
    }

    void TearDown() override {
        manager.reset();
    }

    std::unique_ptr<NetworkManager> manager;
};

// Test network manager initialization
TEST_F(NetworkManagerTest, Initialization) {
    EXPECT_TRUE(manager->is_running());
    EXPECT_GT(manager->get_core_count(), 0);
}

// Test connection management
TEST_F(NetworkManagerTest, ConnectionManagement) {
    NetworkManager::ConnectionConfig config;
    config.host = "localhost";
    config.port = 8080;
    config.buffer_size = 4096;
    config.max_connections = 100;
    config.timeout = 5000;

    // Create connection
    size_t conn_id = manager->connect(config);
    EXPECT_GT(conn_id, 0);

    // Verify connection is active
    EXPECT_TRUE(manager->is_connection_active(conn_id));

    // Close connection
    manager->disconnect(conn_id);
    EXPECT_FALSE(manager->is_connection_active(conn_id));
}

// Test data transfer
TEST_F(NetworkManagerTest, DataTransfer) {
    NetworkManager::ConnectionConfig config;
    config.host = "localhost";
    config.port = 8080;
    config.buffer_size = 4096;
    config.max_connections = 100;
    config.timeout = 5000;

    size_t conn_id = manager->connect(config);
    EXPECT_GT(conn_id, 0);

    // Send data
    const char* test_data = "Hello, Network!";
    size_t sent = manager->send(conn_id, test_data, strlen(test_data));
    EXPECT_EQ(sent, strlen(test_data));

    // Receive data
    char buffer[256] = {0};
    size_t received = manager->receive(conn_id, buffer, sizeof(buffer));
    EXPECT_GT(received, 0);

    manager->disconnect(conn_id);
}

// Test connection pooling
TEST_F(NetworkManagerTest, ConnectionPooling) {
    NetworkManager::ConnectionConfig config;
    config.host = "localhost";
    config.port = 8080;
    config.buffer_size = 4096;
    config.max_connections = 10;
    config.timeout = 5000;

    // Create multiple connections
    std::vector<size_t> conn_ids;
    for (int i = 0; i < 5; i++) {
        size_t conn_id = manager->connect(config);
        EXPECT_GT(conn_id, 0);
        conn_ids.push_back(conn_id);
    }

    // Verify pool size
    EXPECT_EQ(manager->get_pool_size(), 5);

    // Close connections
    for (size_t conn_id : conn_ids) {
        manager->disconnect(conn_id);
    }

    // Verify pool is empty
    EXPECT_EQ(manager->get_pool_size(), 0);
}

// Test performance metrics
TEST_F(NetworkManagerTest, PerformanceMetrics) {
    // Get initial metrics
    auto initial_metrics = manager->get_metrics();

    // Perform some network operations
    NetworkManager::ConnectionConfig config;
    config.host = "localhost";
    config.port = 8080;
    config.buffer_size = 4096;
    config.max_connections = 100;
    config.timeout = 5000;

    for (int i = 0; i < 100; i++) {
        size_t conn_id = manager->connect(config);
        const char* test_data = "Performance test data";
        manager->send(conn_id, test_data, strlen(test_data));
        manager->disconnect(conn_id);
    }

    // Get final metrics
    auto final_metrics = manager->get_metrics();

    // Verify metrics have changed
    EXPECT_GT(final_metrics.connections_established, initial_metrics.connections_established);
    EXPECT_GT(final_metrics.data_sent, initial_metrics.data_sent);
    EXPECT_GT(final_metrics.data_received, initial_metrics.data_received);
}

// Test fault tolerance
TEST_F(NetworkManagerTest, FaultTolerance) {
    // Simulate connection failure
    NetworkManager::ConnectionConfig config;
    config.host = "localhost";
    config.port = 8080;
    config.buffer_size = 4096;
    config.max_connections = 100;
    config.timeout = 5000;

    size_t conn_id = manager->connect(config);
    manager->simulate_connection_failure(conn_id);

    // Verify connection is marked as failed
    EXPECT_FALSE(manager->is_connection_active(conn_id));

    // Attempt to recover
    EXPECT_TRUE(manager->recover_connection(conn_id));
    EXPECT_TRUE(manager->is_connection_active(conn_id));

    manager->disconnect(conn_id);
}

// Test load balancing
TEST_F(NetworkManagerTest, LoadBalancing) {
    // Get initial load distribution
    auto initial_load = manager->get_load_distribution();

    // Create multiple connections to create load
    NetworkManager::ConnectionConfig config;
    config.host = "localhost";
    config.port = 8080;
    config.buffer_size = 4096;
    config.max_connections = 100;
    config.timeout = 5000;

    std::vector<size_t> conn_ids;
    for (int i = 0; i < 50; i++) {
        size_t conn_id = manager->connect(config);
        conn_ids.push_back(conn_id);
    }

    // Wait for load balancing
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Get final load distribution
    auto final_load = manager->get_load_distribution();

    // Verify load is more balanced
    EXPECT_LT(final_load.max_load - final_load.min_load, 
             initial_load.max_load - initial_load.min_load);

    // Cleanup
    for (size_t conn_id : conn_ids) {
        manager->disconnect(conn_id);
    }
}

// Test connection optimization
TEST_F(NetworkManagerTest, ConnectionOptimization) {
    NetworkManager::ConnectionConfig config;
    config.host = "localhost";
    config.port = 8080;
    config.buffer_size = 4096;
    config.max_connections = 100;
    config.timeout = 5000;

    size_t conn_id = manager->connect(config);

    // Get initial connection metrics
    auto initial_metrics = manager->get_connection_metrics(conn_id);

    // Perform some operations
    for (int i = 0; i < 100; i++) {
        const char* test_data = "Optimization test data";
        manager->send(conn_id, test_data, strlen(test_data));
    }

    // Wait for optimization
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Get final connection metrics
    auto final_metrics = manager->get_connection_metrics(conn_id);

    // Verify optimization
    EXPECT_GT(final_metrics.throughput, initial_metrics.throughput);
    EXPECT_LT(final_metrics.latency, initial_metrics.latency);

    manager->disconnect(conn_id);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 