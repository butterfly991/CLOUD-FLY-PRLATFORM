#pragma once

#include <memory>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <string>
#include <functional>
#include "core/Task.h"

namespace core {

class NetworkManager {
public:
    struct NetworkMetrics {
        double bandwidth_usage;
        double latency;
        size_t active_connections;
        size_t queued_requests;
        size_t failed_requests;
    };

    struct ConnectionConfig {
        std::string host;
        uint16_t port;
        size_t buffer_size;
        size_t max_connections;
        size_t timeout_ms;
    };

    NetworkManager();
    ~NetworkManager();

    // Core management
    void start();
    void stop();
    void pause();
    void resume();

    // Connection management
    void initialize_core(size_t core_id);
    void start_core(size_t core_id);
    void stop_core(size_t core_id);
    void pause_core(size_t core_id);
    void resume_core(size_t core_id);

    // Network operations
    size_t connect(const ConnectionConfig& config);
    void disconnect(size_t connection_id);
    size_t send(size_t connection_id, const void* data, size_t size);
    size_t receive(size_t connection_id, void* buffer, size_t size);
    void broadcast(const void* data, size_t size);

    // Task management
    size_t submit_task(size_t task_id, const Task& task);
    void cancel_task(size_t task_id);
    TaskStatus get_task_status(size_t task_id) const;

    // Resource management
    void adjust_resources(size_t core_id);
    void optimize_core_performance(size_t core_id);
    void optimize_core_cache(size_t core_id);
    void configure_core_memory(size_t core_id);

    // State management
    void backup_core_state(size_t core_id);
    void restore_core_state(size_t core_id);
    void cleanup_core(size_t core_id);

    // Metrics
    NetworkMetrics get_metrics() const;
    void update_metrics(const NetworkMetrics& metrics);

private:
    // Core components
    std::vector<std::unique_ptr<class NetworkCore>> cores_;
    std::atomic<bool> running_;
    std::atomic<bool> paused_;
    mutable std::mutex state_mutex_;
    std::condition_variable state_cv_;

    // Connection management
    std::unordered_map<size_t, std::unique_ptr<class Connection>> connections_;
    mutable std::mutex connection_mutex_;

    // Task management
    std::unordered_map<size_t, Task> tasks_;
    std::unordered_map<size_t, TaskStatus> task_status_;
    mutable std::mutex task_mutex_;

    // Metrics
    NetworkMetrics metrics_;
    mutable std::mutex metrics_mutex_;

    // Internal methods
    void monitor_connections();
    void handle_connection_failure(size_t connection_id);
    void optimize_connection(size_t connection_id);
    void cleanup_connection(size_t connection_id);
};

} // namespace core 