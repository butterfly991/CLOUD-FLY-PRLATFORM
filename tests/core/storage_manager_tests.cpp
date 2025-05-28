#include <gtest/gtest.h>
#include "core/StorageManager.h"
#include "core/drivers/memory_ops.h"
#include "core/drivers/thread_ops.h"

using namespace core;

class StorageManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<StorageManager>();
        EXPECT_TRUE(manager->start());
    }

    void TearDown() override {
        manager.reset();
    }

    std::unique_ptr<StorageManager> manager;
};

// Test storage manager initialization
TEST_F(StorageManagerTest, Initialization) {
    EXPECT_TRUE(manager->is_running());
    EXPECT_GT(manager->get_core_count(), 0);
}

// Test file operations
TEST_F(StorageManagerTest, FileOperations) {
    StorageManager::StorageConfig config;
    config.path = "/tmp/test";
    config.block_size = 4096;
    config.cache_size = 1024 * 1024;
    config.max_files = 100;
    config.timeout = 5000;

    // Create file
    size_t file_id = manager->open_file("test.txt", config);
    EXPECT_GT(file_id, 0);

    // Write data
    const char* test_data = "Hello, Storage!";
    size_t written = manager->write_file(file_id, test_data, strlen(test_data), 0);
    EXPECT_EQ(written, strlen(test_data));

    // Read data
    char buffer[256] = {0};
    size_t read = manager->read_file(file_id, buffer, sizeof(buffer), 0);
    EXPECT_EQ(read, strlen(test_data));
    EXPECT_STREQ(buffer, test_data);

    // Close file
    manager->close_file(file_id);
}

// Test file caching
TEST_F(StorageManagerTest, FileCaching) {
    StorageManager::StorageConfig config;
    config.path = "/tmp/test";
    config.block_size = 4096;
    config.cache_size = 1024 * 1024;
    config.max_files = 100;
    config.timeout = 5000;

    size_t file_id = manager->open_file("cache_test.txt", config);

    // Write data multiple times to test cache
    const char* test_data = "Cache test data";
    for (int i = 0; i < 100; i++) {
        size_t written = manager->write_file(file_id, test_data, strlen(test_data), 0);
        EXPECT_EQ(written, strlen(test_data));
    }

    // Get cache metrics
    auto cache_metrics = manager->get_cache_metrics();
    EXPECT_GT(cache_metrics.hit_count, 0);

    manager->close_file(file_id);
}

// Test performance metrics
TEST_F(StorageManagerTest, PerformanceMetrics) {
    // Get initial metrics
    auto initial_metrics = manager->get_metrics();

    // Perform some storage operations
    StorageManager::StorageConfig config;
    config.path = "/tmp/test";
    config.block_size = 4096;
    config.cache_size = 1024 * 1024;
    config.max_files = 100;
    config.timeout = 5000;

    for (int i = 0; i < 100; i++) {
        size_t file_id = manager->open_file("perf_test_" + std::to_string(i) + ".txt", config);
        const char* test_data = "Performance test data";
        manager->write_file(file_id, test_data, strlen(test_data), 0);
        manager->close_file(file_id);
    }

    // Get final metrics
    auto final_metrics = manager->get_metrics();

    // Verify metrics have changed
    EXPECT_GT(final_metrics.files_opened, initial_metrics.files_opened);
    EXPECT_GT(final_metrics.bytes_written, initial_metrics.bytes_written);
    EXPECT_GT(final_metrics.bytes_read, initial_metrics.bytes_read);
}

// Test fault tolerance
TEST_F(StorageManagerTest, FaultTolerance) {
    StorageManager::StorageConfig config;
    config.path = "/tmp/test";
    config.block_size = 4096;
    config.cache_size = 1024 * 1024;
    config.max_files = 100;
    config.timeout = 5000;

    size_t file_id = manager->open_file("fault_test.txt", config);
    manager->simulate_file_failure(file_id);

    // Verify file is marked as failed
    EXPECT_FALSE(manager->is_file_healthy(file_id));

    // Attempt to recover
    EXPECT_TRUE(manager->recover_file(file_id));
    EXPECT_TRUE(manager->is_file_healthy(file_id));

    manager->close_file(file_id);
}

// Test load balancing
TEST_F(StorageManagerTest, LoadBalancing) {
    // Get initial load distribution
    auto initial_load = manager->get_load_distribution();

    // Create multiple files to create load
    StorageManager::StorageConfig config;
    config.path = "/tmp/test";
    config.block_size = 4096;
    config.cache_size = 1024 * 1024;
    config.max_files = 100;
    config.timeout = 5000;

    std::vector<size_t> file_ids;
    for (int i = 0; i < 50; i++) {
        size_t file_id = manager->open_file("load_test_" + std::to_string(i) + ".txt", config);
        file_ids.push_back(file_id);
    }

    // Wait for load balancing
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Get final load distribution
    auto final_load = manager->get_load_distribution();

    // Verify load is more balanced
    EXPECT_LT(final_load.max_load - final_load.min_load, 
             initial_load.max_load - initial_load.min_load);

    // Cleanup
    for (size_t file_id : file_ids) {
        manager->close_file(file_id);
    }
}

// Test file optimization
TEST_F(StorageManagerTest, FileOptimization) {
    StorageManager::StorageConfig config;
    config.path = "/tmp/test";
    config.block_size = 4096;
    config.cache_size = 1024 * 1024;
    config.max_files = 100;
    config.timeout = 5000;

    size_t file_id = manager->open_file("optimization_test.txt", config);

    // Get initial file metrics
    auto initial_metrics = manager->get_file_metrics(file_id);

    // Perform some operations
    for (int i = 0; i < 100; i++) {
        const char* test_data = "Optimization test data";
        manager->write_file(file_id, test_data, strlen(test_data), 0);
    }

    // Wait for optimization
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Get final file metrics
    auto final_metrics = manager->get_file_metrics(file_id);

    // Verify optimization
    EXPECT_GT(final_metrics.throughput, initial_metrics.throughput);
    EXPECT_LT(final_metrics.latency, initial_metrics.latency);

    manager->close_file(file_id);
}

// Test memory management
TEST_F(StorageManagerTest, MemoryManagement) {
    // Get initial memory usage
    auto initial_memory = manager->get_memory_usage();

    // Allocate memory
    for (int i = 0; i < 5; i++) {
        size_t size = 1024 * 1024; // 1MB
        void* ptr = manager->allocate_memory(size);
        EXPECT_NE(ptr, nullptr);
        manager->free_memory(ptr);
    }

    // Get final memory usage
    auto final_memory = manager->get_memory_usage();

    // Verify memory is properly managed
    EXPECT_EQ(final_memory.allocated, initial_memory.allocated);
    EXPECT_EQ(final_memory.fragmentation, initial_memory.fragmentation);
}

// Test concurrent access
TEST_F(StorageManagerTest, ConcurrentAccess) {
    StorageManager::StorageConfig config;
    config.path = "/tmp/test";
    config.block_size = 4096;
    config.cache_size = 1024 * 1024;
    config.max_files = 100;
    config.timeout = 5000;

    size_t file_id = manager->open_file("concurrent_test.txt", config);

    // Create multiple threads to access the file
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([this, file_id, i]() {
            const char* test_data = "Concurrent test data " + std::to_string(i);
            manager->write_file(file_id, test_data, strlen(test_data), i * 100);
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify file integrity
    char buffer[1024] = {0};
    size_t read = manager->read_file(file_id, buffer, sizeof(buffer), 0);
    EXPECT_GT(read, 0);

    manager->close_file(file_id);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 