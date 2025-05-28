#include <gtest/gtest.h>
#include "core/LoadBalancer.h"
#include "core/drivers/compute_ops.h"
#include "core/drivers/memory_ops.h"
#include "core/drivers/thread_ops.h"

using namespace core;

class LoadBalancerTest : public ::testing::Test {
protected:
    void SetUp() override {
        balancer = std::make_unique<LoadBalancer>();
        EXPECT_TRUE(balancer->start());
    }

    void TearDown() override {
        balancer.reset();
    }

    std::unique_ptr<LoadBalancer> balancer;
};

// Test load balancer initialization
TEST_F(LoadBalancerTest, Initialization) {
    EXPECT_TRUE(balancer->is_running());
    EXPECT_GT(balancer->get_core_count(), 0);
}

// Test task distribution
TEST_F(LoadBalancerTest, TaskDistribution) {
    // Submit tasks with different priorities
    for (int i = 0; i < 10; i++) {
        Task task;
        task.type = TaskType::COMPUTE;
        task.priority = static_cast<TaskPriority>(i % 3); // Cycle through priorities
        task.data = "Test task " + std::to_string(i);
        
        size_t task_id = balancer->submit_task(task);
        EXPECT_GT(task_id, 0);

        // Verify task is assigned to a core
        size_t core_id = balancer->get_task_core(task_id);
        EXPECT_LT(core_id, balancer->get_core_count());
    }
}

// Test load balancing
TEST_F(LoadBalancerTest, LoadBalancing) {
    // Get initial load distribution
    auto initial_load = balancer->get_load_distribution();

    // Submit tasks to create load imbalance
    for (int i = 0; i < 100; i++) {
        Task task;
        task.type = TaskType::COMPUTE;
        task.priority = TaskPriority::HIGH;
        task.data = "Load test task " + std::to_string(i);
        balancer->submit_task(task);
    }

    // Wait for load balancing
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Get final load distribution
    auto final_load = balancer->get_load_distribution();

    // Verify load is more balanced
    EXPECT_LT(final_load.max_load - final_load.min_load, 
             initial_load.max_load - initial_load.min_load);
}

// Test resource management
TEST_F(LoadBalancerTest, ResourceManagement) {
    // Get initial resource usage
    auto initial_resources = balancer->get_resource_usage();

    // Allocate resources
    for (int i = 0; i < 5; i++) {
        ResourceRequest request;
        request.type = ResourceType::MEMORY;
        request.amount = 1024 * 1024; // 1MB
        request.priority = ResourcePriority::HIGH;
        
        size_t resource_id = balancer->allocate_resources(request);
        EXPECT_GT(resource_id, 0);
    }

    // Get final resource usage
    auto final_resources = balancer->get_resource_usage();

    // Verify resource allocation
    EXPECT_GT(final_resources.memory_used, initial_resources.memory_used);

    // Release resources
    balancer->release_resources();

    // Verify resources are released
    auto released_resources = balancer->get_resource_usage();
    EXPECT_EQ(released_resources.memory_used, 0);
}

// Test performance optimization
TEST_F(LoadBalancerTest, PerformanceOptimization) {
    // Get initial performance metrics
    auto initial_metrics = balancer->get_metrics();

    // Run some compute-intensive tasks
    for (int i = 0; i < 100; i++) {
        Task task;
        task.type = TaskType::COMPUTE;
        task.priority = TaskPriority::HIGH;
        task.data = "Performance test task " + std::to_string(i);
        balancer->submit_task(task);
    }

    // Wait for tasks to complete
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Get final metrics
    auto final_metrics = balancer->get_metrics();

    // Verify performance improvements
    EXPECT_GT(final_metrics.tasks_processed, initial_metrics.tasks_processed);
    EXPECT_GT(final_metrics.throughput, initial_metrics.throughput);
}

// Test fault tolerance
TEST_F(LoadBalancerTest, FaultTolerance) {
    // Simulate core failure
    balancer->simulate_core_failure(0);

    // Verify system continues to operate
    Task task;
    task.type = TaskType::COMPUTE;
    task.priority = TaskPriority::HIGH;
    task.data = "Fault tolerance test";
    
    size_t task_id = balancer->submit_task(task);
    EXPECT_GT(task_id, 0);

    // Wait for task to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    TaskStatus status = balancer->get_task_status(task_id);
    EXPECT_EQ(status, TaskStatus::COMPLETED);

    // Verify core was recovered
    EXPECT_TRUE(balancer->is_core_healthy(0));
}

// Test task prioritization
TEST_F(LoadBalancerTest, TaskPrioritization) {
    // Submit tasks with different priorities
    std::vector<size_t> task_ids;
    for (int i = 0; i < 10; i++) {
        Task task;
        task.type = TaskType::COMPUTE;
        task.priority = static_cast<TaskPriority>(i % 3);
        task.data = "Priority test task " + std::to_string(i);
        
        size_t task_id = balancer->submit_task(task);
        task_ids.push_back(task_id);
    }

    // Wait for tasks to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify high priority tasks are processed first
    for (size_t i = 0; i < task_ids.size(); i++) {
        TaskStatus status = balancer->get_task_status(task_ids[i]);
        if (i < 3) { // High priority tasks
            EXPECT_EQ(status, TaskStatus::COMPLETED);
        }
    }
}

// Test dynamic scaling
TEST_F(LoadBalancerTest, DynamicScaling) {
    // Get initial core count
    size_t initial_cores = balancer->get_core_count();

    // Increase load
    for (int i = 0; i < 1000; i++) {
        Task task;
        task.type = TaskType::COMPUTE;
        task.priority = TaskPriority::HIGH;
        task.data = "Scaling test task " + std::to_string(i);
        balancer->submit_task(task);
    }

    // Wait for scaling
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Verify core count increased
    EXPECT_GT(balancer->get_core_count(), initial_cores);

    // Reduce load
    balancer->cancel_all_tasks();

    // Wait for scaling down
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Verify core count decreased
    EXPECT_LT(balancer->get_core_count(), balancer->get_max_core_count());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 