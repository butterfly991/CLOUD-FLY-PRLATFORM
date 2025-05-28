#include "core/NetworkManager.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

namespace core {

NetworkManager::NetworkManager()
    : running_(false)
    , paused_(false) {
    metrics_ = NetworkMetrics{};
}

NetworkManager::~NetworkManager() {
    stop();
}

void NetworkManager::start() {
    if (running_.exchange(true)) {
        return;
    }

    // Start monitoring thread
    std::thread monitor_thread(&NetworkManager::monitor_connections, this);
    monitor_thread.detach();
}

void NetworkManager::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    // Wait for monitoring thread to stop
    std::unique_lock<std::mutex> lock(state_mutex_);
    state_cv_.wait(lock, [this] { return !running_; });

    // Cleanup all connections
    std::lock_guard<std::mutex> conn_lock(connection_mutex_);
    for (auto& [id, conn] : connections_) {
        cleanup_connection(id);
    }
    connections_.clear();
}

void NetworkManager::pause() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    paused_ = true;
}

void NetworkManager::resume() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    paused_ = false;
    state_cv_.notify_all();
}

void NetworkManager::initialize_core(size_t core_id) {
    if (core_id >= cores_.size()) {
        cores_.resize(core_id + 1);
    }
    cores_[core_id] = std::make_unique<NetworkCore>();
}

void NetworkManager::start_core(size_t core_id) {
    if (core_id < cores_.size() && cores_[core_id]) {
        cores_[core_id]->start();
    }
}

void NetworkManager::stop_core(size_t core_id) {
    if (core_id < cores_.size() && cores_[core_id]) {
        cores_[core_id]->stop();
    }
}

void NetworkManager::pause_core(size_t core_id) {
    if (core_id < cores_.size() && cores_[core_id]) {
        cores_[core_id]->pause();
    }
}

void NetworkManager::resume_core(size_t core_id) {
    if (core_id < cores_.size() && cores_[core_id]) {
        cores_[core_id]->resume();
    }
}

size_t NetworkManager::connect(const ConnectionConfig& config) {
    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    // Set non-blocking mode
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    // Connect
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config.port);
    inet_pton(AF_INET, config.host.c_str(), &addr.sin_addr);

    int ret = ::connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0 && errno != EINPROGRESS) {
        close(sock);
        throw std::runtime_error("Failed to connect");
    }

    // Create connection object
    std::lock_guard<std::mutex> lock(connection_mutex_);
    size_t connection_id = connections_.size() + 1;
    connections_[connection_id] = std::make_unique<Connection>(sock, config);

    return connection_id;
}

void NetworkManager::disconnect(size_t connection_id) {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    auto it = connections_.find(connection_id);
    if (it != connections_.end()) {
        cleanup_connection(connection_id);
        connections_.erase(it);
    }
}

size_t NetworkManager::send(size_t connection_id, const void* data, size_t size) {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    auto it = connections_.find(connection_id);
    if (it == connections_.end()) {
        return 0;
    }

    return it->second->send(data, size);
}

size_t NetworkManager::receive(size_t connection_id, void* buffer, size_t size) {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    auto it = connections_.find(connection_id);
    if (it == connections_.end()) {
        return 0;
    }

    return it->second->receive(buffer, size);
}

void NetworkManager::broadcast(const void* data, size_t size) {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    for (auto& [id, conn] : connections_) {
        conn->send(data, size);
    }
}

size_t NetworkManager::submit_task(size_t task_id, const Task& task) {
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

void NetworkManager::cancel_task(size_t task_id) {
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

TaskStatus NetworkManager::get_task_status(size_t task_id) const {
    std::lock_guard<std::mutex> lock(task_mutex_);
    auto it = task_status_.find(task_id);
    return it != task_status_.end() ? it->second : TaskStatus::UNKNOWN;
}

void NetworkManager::adjust_resources(size_t core_id) {
    if (core_id < cores_.size() && cores_[core_id]) {
        cores_[core_id]->adjust_resources();
    }
}

void NetworkManager::optimize_core_performance(size_t core_id) {
    if (core_id < cores_.size() && cores_[core_id]) {
        cores_[core_id]->optimize_performance();
    }
}

void NetworkManager::optimize_core_cache(size_t core_id) {
    if (core_id < cores_.size() && cores_[core_id]) {
        cores_[core_id]->optimize_cache();
    }
}

void NetworkManager::configure_core_memory(size_t core_id) {
    if (core_id < cores_.size() && cores_[core_id]) {
        cores_[core_id]->configure_memory();
    }
}

void NetworkManager::backup_core_state(size_t core_id) {
    if (core_id < cores_.size() && cores_[core_id]) {
        cores_[core_id]->backup_state();
    }
}

void NetworkManager::restore_core_state(size_t core_id) {
    if (core_id < cores_.size() && cores_[core_id]) {
        cores_[core_id]->restore_state();
    }
}

void NetworkManager::cleanup_core(size_t core_id) {
    if (core_id < cores_.size() && cores_[core_id]) {
        cores_[core_id]->cleanup();
    }
}

NetworkManager::NetworkMetrics NetworkManager::get_metrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return metrics_;
}

void NetworkManager::update_metrics(const NetworkMetrics& metrics) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    metrics_ = metrics;
}

void NetworkManager::monitor_connections() {
    while (running_) {
        {
            std::unique_lock<std::mutex> lock(state_mutex_);
            state_cv_.wait(lock, [this] { return !paused_ || !running_; });
            
            if (!running_) {
                break;
            }
        }

        // Update metrics
        NetworkMetrics new_metrics{};
        {
            std::lock_guard<std::mutex> lock(connection_mutex_);
            for (const auto& [id, conn] : connections_) {
                auto conn_metrics = conn->get_metrics();
                new_metrics.bandwidth_usage += conn_metrics.bandwidth_usage;
                new_metrics.latency += conn_metrics.latency;
                new_metrics.active_connections++;
                new_metrics.queued_requests += conn_metrics.queued_requests;
                new_metrics.failed_requests += conn_metrics.failed_requests;
            }
        }

        // Normalize metrics
        if (new_metrics.active_connections > 0) {
            new_metrics.bandwidth_usage /= new_metrics.active_connections;
            new_metrics.latency /= new_metrics.active_connections;
        }

        // Update global metrics
        update_metrics(new_metrics);

        // Check connection health
        std::lock_guard<std::mutex> lock(connection_mutex_);
        for (const auto& [id, conn] : connections_) {
            if (!conn->is_healthy()) {
                handle_connection_failure(id);
            } else {
                optimize_connection(id);
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void NetworkManager::handle_connection_failure(size_t connection_id) {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    auto it = connections_.find(connection_id);
    if (it != connections_.end()) {
        // Attempt recovery
        if (!it->second->recover()) {
            // If recovery fails, cleanup
            cleanup_connection(connection_id);
            connections_.erase(it);
        }
    }
}

void NetworkManager::optimize_connection(size_t connection_id) {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    auto it = connections_.find(connection_id);
    if (it != connections_.end()) {
        it->second->optimize();
    }
}

void NetworkManager::cleanup_connection(size_t connection_id) {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    auto it = connections_.find(connection_id);
    if (it != connections_.end()) {
        it->second->cleanup();
    }
}

} // namespace core 