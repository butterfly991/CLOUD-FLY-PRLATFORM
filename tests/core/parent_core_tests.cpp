#include <gtest/gtest.h>
#include "core/ParentCore.h"
#include "core/drivers/compute_ops.h"
#include "core/drivers/memory_ops.h"
#include "core/drivers/thread_ops.h"
#include "core/drivers/cpu_info.h"

using namespace core;

class ParentCoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        core = std::make_unique<ParentCore>();
        EXPECT_TRUE(core->initialize());
    }

    void TearDown() override {
        core.reset();
    }

    std::unique_ptr<ParentCore> core;
};

// Test core initialization
TEST_F(ParentCoreTest, Initialization) {
    EXPECT_TRUE(core->is_initialized());
    EXPECT_GT(core->get_core_count(), 0);
}

// Test core management
TEST_F(ParentCoreTest, CoreManagement) {
    // Start all cores
    EXPECT_TRUE(core->start());
    EXPECT_TRUE(core->is_running());

    // Pause and resume
    core->pause();
    EXPECT_TRUE(core->is_paused());
    
    core->resume();
    EXPECT_FALSE(core->is_paused());

    // Stop
    core->stop();
    EXPECT_FALSE(core->is_running());
}

// Test resource management
TEST_F(ParentCoreTest, ResourceManagement) {
    EXPECT_TRUE(core->start());

    // Get initial resource usage
    auto initial_resources = core->get_resource_usage();

    // Allocate resources
    for (int i = 0; i < 5; i++) {
        ResourceRequest request;
        request.type = ResourceType::MEMORY;
        request.amount = 1024 * 1024; // 1MB
        request.priority = ResourcePriority::HIGH;
        
        size_t resource_id = core->allocate_resources(request);
        EXPECT_GT(resource_id, 0);
    }

    // Get final resource usage
    auto final_resources = core->get_resource_usage();

    // Verify resource allocation
    EXPECT_GT(final_resources.memory_used, initial_resources.memory_used);

    // Release resources
    core->release_resources();

    // Verify resources are released
    auto released_resources = core->get_resource_usage();
    EXPECT_EQ(released_resources.memory_used, 0);
}

// Test task management
TEST_F(ParentCoreTest, TaskManagement) {
    EXPECT_TRUE(core->start());

    // Submit tasks
    std::vector<size_t> task_ids;
    for (int i = 0; i < 10; i++) {
        Task task;
        task.type = TaskType::COMPUTE;
        task.priority = static_cast<TaskPriority>(i % 3);
        task.data = "Test task " + std::to_string(i);
        
        size_t task_id = core->submit_task(task);
        EXPECT_GT(task_id, 0);
        task_ids.push_back(task_id);
    }

    // Wait for tasks to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify task completion
    for (size_t task_id : task_ids) {
        TaskStatus status = core->get_task_status(task_id);
        EXPECT_EQ(status, TaskStatus::COMPLETED);
    }
}

// Test performance optimization
TEST_F(ParentCoreTest, PerformanceOptimization) {
    EXPECT_TRUE(core->start());

    // Get initial performance metrics
    auto initial_metrics = core->get_metrics();

    // Run some compute-intensive tasks
    for (int i = 0; i < 100; i++) {
        Task task;
        task.type = TaskType::COMPUTE;
        task.priority = TaskPriority::HIGH;
        task.data = "Performance test task " + std::to_string(i);
        core->submit_task(task);
    }

    // Wait for tasks to complete
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Get final metrics
    auto final_metrics = core->get_metrics();

    // Verify performance improvements
    EXPECT_GT(final_metrics.tasks_processed, initial_metrics.tasks_processed);
    EXPECT_GT(final_metrics.throughput, initial_metrics.throughput);
}

// Test fault tolerance
TEST_F(ParentCoreTest, FaultTolerance) {
    EXPECT_TRUE(core->start());

    // Simulate core failure
    core->simulate_core_failure(0);

    // Verify system continues to operate
    Task task;
    task.type = TaskType::COMPUTE;
    task.priority = TaskPriority::HIGH;
    task.data = "Fault tolerance test";
    
    size_t task_id = core->submit_task(task);
    EXPECT_GT(task_id, 0);

    // Wait for task to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    TaskStatus status = core->get_task_status(task_id);
    EXPECT_EQ(status, TaskStatus::COMPLETED);

    // Verify core was recovered
    EXPECT_TRUE(core->is_core_healthy(0));
}

// Test inter-core communication
TEST_F(ParentCoreTest, InterCoreCommunication) {
    EXPECT_TRUE(core->start());

    // Create message
    Message msg;
    msg.type = MessageType::DATA;
    msg.data = "Test message";
    msg.sender = 0;
    msg.receiver = 1;

    // Send message
    EXPECT_TRUE(core->send_message(msg));

    // Wait for message to be received
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify message was received
    auto received_messages = core->get_received_messages(1);
    EXPECT_FALSE(received_messages.empty());
    EXPECT_EQ(received_messages[0].data, msg.data);
}

// Test state management
TEST_F(ParentCoreTest, StateManagement) {
    EXPECT_TRUE(core->start());

    // Create initial state
    std::string state = "Initial state";
    EXPECT_TRUE(core->verify_state(state));

    // Backup state
    EXPECT_TRUE(core->backup_state());

    // Modify state
    state = "Modified state";
    EXPECT_TRUE(core->verify_state(state));

    // Restore state
    EXPECT_TRUE(core->restore_state());
    EXPECT_TRUE(core->verify_state("Initial state"));
}

// Test monitoring
TEST_F(ParentCoreTest, Monitoring) {
    EXPECT_TRUE(core->start());

    // Get initial metrics
    auto initial_metrics = core->get_metrics();

    // Perform some operations
    for (int i = 0; i < 100; i++) {
        Task task;
        task.type = TaskType::COMPUTE;
        task.priority = TaskPriority::HIGH;
        task.data = "Monitoring test task " + std::to_string(i);
        core->submit_task(task);
    }

    // Wait for monitoring
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Get final metrics
    auto final_metrics = core->get_metrics();

    // Verify metrics have changed
    EXPECT_GT(final_metrics.tasks_processed, initial_metrics.tasks_processed);
    EXPECT_GT(final_metrics.throughput, initial_metrics.throughput);
    EXPECT_GT(final_metrics.cpu_usage, initial_metrics.cpu_usage);
    EXPECT_GT(final_metrics.memory_usage, initial_metrics.memory_usage);
}

// Test hardware acceleration
TEST_F(ParentCoreTest, HardwareAcceleration) {
    EXPECT_TRUE(core->start());

    // Get initial acceleration metrics
    auto initial_metrics = core->get_acceleration_metrics();

    // Enable hardware acceleration
    EXPECT_TRUE(core->enable_hardware_acceleration());

    // Run some compute-intensive tasks
    for (int i = 0; i < 100; i++) {
        Task task;
        task.type = TaskType::COMPUTE;
        task.priority = TaskPriority::HIGH;
        task.data = "Acceleration test task " + std::to_string(i);
        core->submit_task(task);
    }

    // Wait for tasks to complete
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Get final acceleration metrics
    auto final_metrics = core->get_acceleration_metrics();

    // Verify acceleration is working
    EXPECT_GT(final_metrics.accelerated_tasks, initial_metrics.accelerated_tasks);
    EXPECT_GT(final_metrics.acceleration_ratio, initial_metrics.acceleration_ratio);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 