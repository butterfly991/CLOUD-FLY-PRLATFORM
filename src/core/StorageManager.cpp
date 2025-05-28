#include "core/StorageManager.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

namespace core {

StorageManager::StorageManager()
    : running_(false)
    , paused_(false) {
    metrics_ = StorageMetrics{};
}

StorageManager::~StorageManager() {
    stop();
}

void StorageManager::start() {
    if (running_.exchange(true)) {
        return;
    }

    // Start monitoring thread
    std::thread monitor_thread(&StorageManager::monitor_storage, this);
    monitor_thread.detach();
}

void StorageManager::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    // Wait for monitoring thread to stop
    std::unique_lock<std::mutex> lock(state_mutex_);
    state_cv_.wait(lock, [this] { return !running_; });

    // Cleanup all files
    std::lock_guard<std::mutex> file_lock(file_mutex_);
    for (auto& [id, file] : files_) {
        cleanup_file(id);
    }
    files_.clear();
}

void StorageManager::pause() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    paused_ = true;
}

void StorageManager::resume() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    paused_ = false;
    state_cv_.notify_all();
}

void StorageManager::initialize_core(size_t core_id) {
    if (core_id >= cores_.size()) {
        cores_.resize(core_id + 1);
    }
    cores_[core_id] = std::make_unique<StorageCore>();
}

void StorageManager::start_core(size_t core_id) {
    if (core_id < cores_.size() && cores_[core_id]) {
        cores_[core_id]->start();
    }
}

void StorageManager::stop_core(size_t core_id) {
    if (core_id < cores_.size() && cores_[core_id]) {
        cores_[core_id]->stop();
    }
}

void StorageManager::pause_core(size_t core_id) {
    if (core_id < cores_.size() && cores_[core_id]) {
        cores_[core_id]->pause();
    }
}

void StorageManager::resume_core(size_t core_id) {
    if (core_id < cores_.size() && cores_[core_id]) {
        cores_[core_id]->resume();
    }
}

size_t StorageManager::open_file(const std::string& path, const StorageConfig& config) {
    // Open file
    int fd = ::open(path.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        throw std::runtime_error("Failed to open file: " + std::string(strerror(errno)));
    }

    // Create file object
    std::lock_guard<std::mutex> lock(file_mutex_);
    size_t file_id = files_.size() + 1;
    files_[file_id] = std::make_unique<File>(fd, config);

    return file_id;
}

void StorageManager::close_file(size_t file_id) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    auto it = files_.find(file_id);
    if (it != files_.end()) {
        cleanup_file(file_id);
        files_.erase(it);
    }
}

size_t StorageManager::read_file(size_t file_id, void* buffer, size_t size, size_t offset) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    auto it = files_.find(file_id);
    if (it == files_.end()) {
        return 0;
    }

    return it->second->read(buffer, size, offset);
}

size_t StorageManager::write_file(size_t file_id, const void* data, size_t size, size_t offset) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    auto it = files_.find(file_id);
    if (it == files_.end()) {
        return 0;
    }

    return it->second->write(data, size, offset);
}

void StorageManager::delete_file(size_t file_id) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    auto it = files_.find(file_id);
    if (it != files_.end()) {
        cleanup_file(file_id);
        files_.erase(it);
    }
}

size_t StorageManager::submit_task(size_t task_id, const Task& task) {
    std::lock_guard<std::mutex> lock(task_mutex_);
    
    // Store task
    tasks_[task_id] = task;
    task_status_[task_id] = TaskStatus::PENDING;

    // Find appropriate core
    size_t target_core = task_id % cores_.size();
    if (target_core < cores_.size() && cores_[target_core]) {
        cores_[target_core]->submit_task(task_id, task);
    }

    return task_id;
}

void StorageManager::cancel_task(size_t task_id) {
    std::lock_guard<std::mutex> lock(task_mutex_);
    
    auto it = tasks_.find(task_id);
    if (it == tasks_.end()) {
        return;
    }

    // Find core
    size_t target_core = task_id % cores_.size();
    if (target_core < cores_.size() && cores_[target_core]) {
        cores_[target_core]->cancel_task(task_id);
    }

    // Update status
    task_status_[task_id] = TaskStatus::CANCELLED;
}

TaskStatus StorageManager::get_task_status(size_t task_id) const {
    std::lock_guard<std::mutex> lock(task_mutex_);
    auto it = task_status_.find(task_id);
    return it != task_status_.end() ? it->second : TaskStatus::UNKNOWN;
}

void StorageManager::adjust_resources(size_t core_id) {
    if (core_id < cores_.size() && cores_[core_id]) {
        cores_[core_id]->adjust_resources();
    }
}

void StorageManager::optimize_core_performance(size_t core_id) {
    if (core_id < cores_.size() && cores_[core_id]) {
        cores_[core_id]->optimize_performance();
    }
}

void StorageManager::optimize_core_cache(size_t core_id) {
    if (core_id < cores_.size() && cores_[core_id]) {
        cores_[core_id]->optimize_cache();
    }
}

void StorageManager::configure_core_memory(size_t core_id) {
    if (core_id < cores_.size() && cores_[core_id]) {
        cores_[core_id]->configure_memory();
    }
}

void StorageManager::compact_memory(size_t core_id) {
    if (core_id < cores_.size() && cores_[core_id]) {
        cores_[core_id]->compact_memory();
    }
}

void StorageManager::backup_core_state(size_t core_id) {
    if (core_id < cores_.size() && cores_[core_id]) {
        cores_[core_id]->backup_state();
    }
}

void StorageManager::restore_core_state(size_t core_id) {
    if (core_id < cores_.size() && cores_[core_id]) {
        cores_[core_id]->restore_state();
    }
}

void StorageManager::cleanup_core(size_t core_id) {
    if (core_id < cores_.size() && cores_[core_id]) {
        cores_[core_id]->cleanup();
    }
}

StorageManager::StorageMetrics StorageManager::get_metrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return metrics_;
}

void StorageManager::update_metrics(const StorageMetrics& metrics) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    metrics_ = metrics;
}

void StorageManager::monitor_storage() {
    while (running_) {
        {
            std::unique_lock<std::mutex> lock(state_mutex_);
            state_cv_.wait(lock, [this] { return !paused_ || !running_; });
            
            if (!running_) {
                break;
            }
        }

        // Update metrics
        StorageMetrics new_metrics{};
        {
            std::lock_guard<std::mutex> lock(file_mutex_);
            for (const auto& [id, file] : files_) {
                auto file_metrics = file->get_metrics();
                new_metrics.disk_usage += file_metrics.disk_usage;
                new_metrics.io_throughput += file_metrics.io_throughput;
                new_metrics.active_operations += file_metrics.active_operations;
                new_metrics.queued_operations += file_metrics.queued_operations;
                new_metrics.failed_operations += file_metrics.failed_operations;
            }
        }

        // Normalize metrics
        if (new_metrics.active_operations > 0) {
            new_metrics.disk_usage /= new_metrics.active_operations;
            new_metrics.io_throughput /= new_metrics.active_operations;
        }

        // Update global metrics
        update_metrics(new_metrics);

        // Check file health
        std::lock_guard<std::mutex> lock(file_mutex_);
        for (const auto& [id, file] : files_) {
            if (!file->is_healthy()) {
                handle_file_failure(id);
            } else {
                optimize_file(id);
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void StorageManager::handle_file_failure(size_t file_id) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    auto it = files_.find(file_id);
    if (it != files_.end()) {
        // Attempt recovery
        if (!it->second->recover()) {
            // If recovery fails, cleanup
            cleanup_file(file_id);
            files_.erase(it);
        }
    }
}

void StorageManager::optimize_file(size_t file_id) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    auto it = files_.find(file_id);
    if (it != files_.end()) {
        it->second->optimize();
    }
}

void StorageManager::cleanup_file(size_t file_id) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    auto it = files_.find(file_id);
    if (it != files_.end()) {
        it->second->cleanup();
    }
}

} // namespace core 