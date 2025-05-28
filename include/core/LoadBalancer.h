#pragma once

#include <memory>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include "core/MultiCoreEngine.h"
#include "core/blockchain/MultiCoreBlockchain.h"
#include "core/NetworkManager.h"
#include "core/StorageManager.h"

namespace core {

class LoadBalancer {
public:
    struct TaskMetrics {
        double cpu_usage;
        double memory_usage;
        double network_usage;
        size_t queue_size;
        size_t active_tasks;
    };

    struct CoreMetrics {
        TaskMetrics metrics;
        std::chrono::steady_clock::time_point last_update;
        bool is_healthy;
    };

    LoadBalancer();
    ~LoadBalancer();

    // Core management
    void start();
    void stop();
    void pause();
    void resume();

    // Component registration
    void set_compute_engine(MultiCoreEngine* engine);
    void set_blockchain_engine(MultiCoreBlockchain* engine);
    void set_network_manager(NetworkManager* manager);
    void set_storage_manager(StorageManager* manager);

    // Task management
    size_t submit_task(const Task& task);
    void cancel_task(size_t task_id);
    TaskStatus get_task_status(size_t task_id) const;
    void update_task_metrics(size_t task_id, const TaskMetrics& metrics);

    // Core management
    size_t find_least_loaded_core() const;
    void update_core_metrics(size_t core_id, const CoreMetrics& metrics);
    bool is_core_healthy(size_t core_id) const;
    void mark_core_unhealthy(size_t core_id);

    // Resource management
    void adjust_resources(size_t core_id);
    void rebalance_load();
    void optimize_distribution();

private:
    // Core components
    MultiCoreEngine* compute_engine_;
    MultiCoreBlockchain* blockchain_engine_;
    NetworkManager* network_manager_;
    StorageManager* storage_manager_;

    // State management
    std::atomic<bool> running_;
    std::atomic<bool> paused_;
    mutable std::mutex state_mutex_;
    std::condition_variable state_cv_;

    // Task management
    std::unordered_map<size_t, Task> tasks_;
    std::unordered_map<size_t, TaskStatus> task_status_;
    std::unordered_map<size_t, TaskMetrics> task_metrics_;
    mutable std::mutex task_mutex_;

    // Core metrics
    std::unordered_map<size_t, CoreMetrics> core_metrics_;
    mutable std::mutex metrics_mutex_;

    // Internal methods
    void monitor_cores();
    void handle_core_failure(size_t core_id);
    void redistribute_tasks(size_t failed_core_id);
    void optimize_core_performance(size_t core_id);
    void adjust_core_resources(size_t core_id);
    void cleanup_core(size_t core_id);
};

} // namespace core 