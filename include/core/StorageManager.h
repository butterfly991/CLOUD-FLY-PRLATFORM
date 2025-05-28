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

class StorageManager {
public:
    struct StorageMetrics {
        double disk_usage;
        double io_throughput;
        size_t active_operations;
        size_t queued_operations;
        size_t failed_operations;
    };

    struct StorageConfig {
        std::string path;
        size_t block_size;
        size_t cache_size;
        size_t max_files;
        size_t timeout_ms;
    };

    StorageManager();
    ~StorageManager();

    // Core management
    void start();
    void stop();
    void pause();
    void resume();

    // Storage management
    void initialize_core(size_t core_id);
    void start_core(size_t core_id);
    void stop_core(size_t core_id);
    void pause_core(size_t core_id);
    void resume_core(size_t core_id);

    // File operations
    size_t open_file(const std::string& path, const StorageConfig& config);
    void close_file(size_t file_id);
    size_t read_file(size_t file_id, void* buffer, size_t size, size_t offset);
    size_t write_file(size_t file_id, const void* data, size_t size, size_t offset);
    void delete_file(size_t file_id);

    // Task management
    size_t submit_task(size_t task_id, const Task& task);
    void cancel_task(size_t task_id);
    TaskStatus get_task_status(size_t task_id) const;

    // Resource management
    void adjust_resources(size_t core_id);
    void optimize_core_performance(size_t core_id);
    void optimize_core_cache(size_t core_id);
    void configure_core_memory(size_t core_id);
    void compact_memory(size_t core_id);

    // State management
    void backup_core_state(size_t core_id);
    void restore_core_state(size_t core_id);
    void cleanup_core(size_t core_id);

    // Metrics
    StorageMetrics get_metrics() const;
    void update_metrics(const StorageMetrics& metrics);

private:
    // Core components
    std::vector<std::unique_ptr<class StorageCore>> cores_;
    std::atomic<bool> running_;
    std::atomic<bool> paused_;
    mutable std::mutex state_mutex_;
    std::condition_variable state_cv_;

    // File management
    std::unordered_map<size_t, std::unique_ptr<class File>> files_;
    mutable std::mutex file_mutex_;

    // Task management
    std::unordered_map<size_t, Task> tasks_;
    std::unordered_map<size_t, TaskStatus> task_status_;
    mutable std::mutex task_mutex_;

    // Metrics
    StorageMetrics metrics_;
    mutable std::mutex metrics_mutex_;

    // Internal methods
    void monitor_storage();
    void handle_file_failure(size_t file_id);
    void optimize_file(size_t file_id);
    void cleanup_file(size_t file_id);
};

} // namespace core 