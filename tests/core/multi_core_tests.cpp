#include <gtest/gtest.h>
#include "core/MultiCoreEngine.h"
#include "core/drivers/compute_ops.h"
#include "core/drivers/memory_ops.h"
#include "core/drivers/thread_ops.h"
#include "core/drivers/cpu_info.h"

using namespace core;

class MultiCoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine = std::make_unique<MultiCoreEngine>();
        EXPECT_TRUE(engine->initialize());
    }

    void TearDown() override {
        engine.reset();
    }

    std::unique_ptr<MultiCoreEngine> engine;
};

// Test engine initialization
TEST_F(MultiCoreTest, Initialization) {
    EXPECT_TRUE(engine->is_initialized());
    EXPECT_EQ(engine->get_core_count(), cpu_info_get_cores());
}

// Test core management
TEST_F(MultiCoreTest, CoreManagement) {
    // Start all cores
    EXPECT_TRUE(engine->start());
    EXPECT_TRUE(engine->is_running());

    // Pause and resume
    engine->pause();
    EXPECT_TRUE(engine->is_paused());
    
    engine->resume();
    EXPECT_FALSE(engine->is_paused());

    // Stop
    engine->stop();
    EXPECT_FALSE(engine->is_running());
}

// Test task distribution
TEST_F(MultiCoreTest, TaskDistribution) {
    EXPECT_TRUE(engine->start());

    // Submit tasks to different cores
    for (int i = 0; i < 10; i++) {
        Task task;
        task.type = TaskType::COMPUTE;
        task.priority = TaskPriority::HIGH;
        task.data = "Test task " + std::to_string(i);
        
        size_t task_id = engine->submit_task(task);
        EXPECT_GT(task_id, 0);

        // Verify task is assigned to a core
        size_t core_id = engine->get_task_core(task_id);
        EXPECT_LT(core_id, engine->get_core_count());
    }

    engine->stop();
}

// Test performance optimization
TEST_F(MultiCoreTest, PerformanceOptimization) {
    EXPECT_TRUE(engine->start());

    // Get initial performance metrics
    auto initial_metrics = engine->get_metrics();

    // Run some compute-intensive tasks
    for (int i = 0; i < 100; i++) {
        Task task;
        task.type = TaskType::COMPUTE;
        task.priority = TaskPriority::HIGH;
        task.data = "Performance test task " + std::to_string(i);
        engine->submit_task(task);
    }

    // Wait for tasks to complete
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Get final metrics
    auto final_metrics = engine->get_metrics();

    // Verify performance improvements
    EXPECT_GT(final_metrics.tasks_processed, initial_metrics.tasks_processed);
    EXPECT_GT(final_metrics.throughput, initial_metrics.throughput);
}

// Test resource management
TEST_F(MultiCoreTest, ResourceManagement) {
    EXPECT_TRUE(engine->start());

    // Get initial resource usage
    auto initial_resources = engine->get_resource_usage();

    // Allocate resources
    for (int i = 0; i < 5; i++) {
        ResourceRequest request;
        request.type = ResourceType::MEMORY;
        request.amount = 1024 * 1024; // 1MB
        request.priority = ResourcePriority::HIGH;
        
        size_t resource_id = engine->allocate_resources(request);
        EXPECT_GT(resource_id, 0);
    }

    // Get final resource usage
    auto final_resources = engine->get_resource_usage();

    // Verify resource allocation
    EXPECT_GT(final_resources.memory_used, initial_resources.memory_used);

    // Release resources
    engine->release_resources();

    // Verify resources are released
    auto released_resources = engine->get_resource_usage();
    EXPECT_EQ(released_resources.memory_used, 0);
}

// Test fault tolerance
TEST_F(MultiCoreTest, FaultTolerance) {
    EXPECT_TRUE(engine->start());

    // Simulate core failure
    engine->simulate_core_failure(0);

    // Verify system continues to operate
    Task task;
    task.type = TaskType::COMPUTE;
    task.priority = TaskPriority::HIGH;
    task.data = "Fault tolerance test";
    
    size_t task_id = engine->submit_task(task);
    EXPECT_GT(task_id, 0);

    // Wait for task to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    TaskStatus status = engine->get_task_status(task_id);
    EXPECT_EQ(status, TaskStatus::COMPLETED);

    // Verify core was recovered
    EXPECT_TRUE(engine->is_core_healthy(0));
}

// Test cache optimization
TEST_F(MultiCoreTest, CacheOptimization) {
    EXPECT_TRUE(engine->start());

    // Get initial cache metrics
    auto initial_cache = engine->get_cache_metrics();

    // Run cache-intensive tasks
    for (int i = 0; i < 1000; i++) {
        Task task;
        task.type = TaskType::COMPUTE;
        task.priority = TaskPriority::HIGH;
        task.data = "Cache test task " + std::to_string(i);
        engine->submit_task(task);
    }

    // Wait for tasks to complete
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Get final cache metrics
    auto final_cache = engine->get_cache_metrics();

    // Verify cache optimization
    EXPECT_GT(final_cache.hit_rate, initial_cache.hit_rate);
    EXPECT_LT(final_cache.miss_rate, initial_cache.miss_rate);
}

// Test thread affinity
TEST_F(MultiCoreTest, ThreadAffinity) {
    EXPECT_TRUE(engine->start());

    // Set thread affinity for each core
    for (size_t i = 0; i < engine->get_core_count(); i++) {
        EXPECT_TRUE(engine->set_thread_affinity(i, i));
    }

    // Verify thread affinity
    for (size_t i = 0; i < engine->get_core_count(); i++) {
        EXPECT_EQ(engine->get_thread_affinity(i), i);
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 