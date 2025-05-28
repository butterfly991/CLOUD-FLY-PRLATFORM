#include "core/LoadBalancer.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>

namespace core {

LoadBalancer::LoadBalancer()
    : compute_engine_(nullptr)
    , blockchain_engine_(nullptr)
    , network_manager_(nullptr)
    , storage_manager_(nullptr)
    , running_(false)
    , paused_(false) {
}

LoadBalancer::~LoadBalancer() {
    stop();
}

void LoadBalancer::start() {
    if (running_.exchange(true)) {
        return;
    }

    // Start monitoring thread
    std::thread monitor_thread(&LoadBalancer::monitor_cores, this);
    monitor_thread.detach();
}

void LoadBalancer::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    // Wait for monitoring thread to stop
    std::unique_lock<std::mutex> lock(state_mutex_);
    state_cv_.wait(lock, [this] { return !running_; });
}

void LoadBalancer::pause() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    paused_ = true;
}

void LoadBalancer::resume() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    paused_ = false;
    state_cv_.notify_all();
}

void LoadBalancer::set_compute_engine(MultiCoreEngine* engine) {
    compute_engine_ = engine;
}

void LoadBalancer::set_blockchain_engine(MultiCoreBlockchain* engine) {
    blockchain_engine_ = engine;
}

void LoadBalancer::set_network_manager(NetworkManager* manager) {
    network_manager_ = manager;
}

void LoadBalancer::set_storage_manager(StorageManager* manager) {
    storage_manager_ = manager;
}

size_t LoadBalancer::submit_task(const Task& task) {
    std::lock_guard<std::mutex> lock(task_mutex_);
    
    // Find least loaded core
    size_t target_core = find_least_loaded_core();
    if (target_core == static_cast<size_t>(-1)) {
        throw std::runtime_error("No available cores for task submission");
    }

    // Generate task ID
    size_t task_id = tasks_.size() + 1;
    
    // Store task
    tasks_[task_id] = task;
    task_status_[task_id] = TaskStatus::PENDING;
    task_metrics_[task_id] = TaskMetrics{};

    // Submit to appropriate engine
    if (task.type == TaskType::COMPUTE) {
        compute_engine_->submit_task(task_id, task);
    } else if (task.type == TaskType::BLOCKCHAIN) {
        blockchain_engine_->submit_task(task_id, task);
    } else if (task.type == TaskType::NETWORK) {
        network_manager_->submit_task(task_id, task);
    } else if (task.type == TaskType::STORAGE) {
        storage_manager_->submit_task(task_id, task);
    }

    return task_id;
}

void LoadBalancer::cancel_task(size_t task_id) {
    std::lock_guard<std::mutex> lock(task_mutex_);
    
    auto it = tasks_.find(task_id);
    if (it == tasks_.end()) {
        return;
    }

    // Cancel task in appropriate engine
    if (it->second.type == TaskType::COMPUTE) {
        compute_engine_->cancel_task(task_id);
    } else if (it->second.type == TaskType::BLOCKCHAIN) {
        blockchain_engine_->cancel_task(task_id);
    } else if (it->second.type == TaskType::NETWORK) {
        network_manager_->cancel_task(task_id);
    } else if (it->second.type == TaskType::STORAGE) {
        storage_manager_->cancel_task(task_id);
    }

    // Update status
    task_status_[task_id] = TaskStatus::CANCELLED;
}

TaskStatus LoadBalancer::get_task_status(size_t task_id) const {
    std::lock_guard<std::mutex> lock(task_mutex_);
    auto it = task_status_.find(task_id);
    return it != task_status_.end() ? it->second : TaskStatus::UNKNOWN;
}

void LoadBalancer::update_task_metrics(size_t task_id, const TaskMetrics& metrics) {
    std::lock_guard<std::mutex> lock(task_mutex_);
    task_metrics_[task_id] = metrics;
}

size_t LoadBalancer::find_least_loaded_core() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    size_t least_loaded_core = static_cast<size_t>(-1);
    double min_load = std::numeric_limits<double>::max();

    for (const auto& [core_id, metrics] : core_metrics_) {
        if (!metrics.is_healthy) {
            continue;
        }

        double load = metrics.metrics.cpu_usage * 0.4 +
                     metrics.metrics.memory_usage * 0.3 +
                     metrics.metrics.network_usage * 0.3;

        if (load < min_load) {
            min_load = load;
            least_loaded_core = core_id;
        }
    }

    return least_loaded_core;
}

void LoadBalancer::update_core_metrics(size_t core_id, const CoreMetrics& metrics) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    core_metrics_[core_id] = metrics;
}

bool LoadBalancer::is_core_healthy(size_t core_id) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    auto it = core_metrics_.find(core_id);
    return it != core_metrics_.end() && it->second.is_healthy;
}

void LoadBalancer::mark_core_unhealthy(size_t core_id) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    auto it = core_metrics_.find(core_id);
    if (it != core_metrics_.end()) {
        it->second.is_healthy = false;
        handle_core_failure(core_id);
    }
}

void LoadBalancer::adjust_resources(size_t core_id) {
    // Get current metrics
    CoreMetrics metrics;
    {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        auto it = core_metrics_.find(core_id);
        if (it == core_metrics_.end()) {
            return;
        }
        metrics = it->second;
    }

    // Adjust resources based on metrics
    if (metrics.metrics.cpu_usage > 0.8) {
        optimize_core_performance(core_id);
    }

    if (metrics.metrics.memory_usage > 0.8) {
        adjust_core_resources(core_id);
    }

    if (metrics.metrics.network_usage > 0.8) {
        rebalance_load();
    }
}

void LoadBalancer::rebalance_load() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    // Find overloaded cores
    std::vector<size_t> overloaded_cores;
    for (const auto& [core_id, metrics] : core_metrics_) {
        if (metrics.metrics.cpu_usage > 0.8 ||
            metrics.metrics.memory_usage > 0.8 ||
            metrics.metrics.network_usage > 0.8) {
            overloaded_cores.push_back(core_id);
        }
    }

    // Redistribute tasks from overloaded cores
    for (size_t core_id : overloaded_cores) {
        redistribute_tasks(core_id);
    }
}

void LoadBalancer::optimize_distribution() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    // Calculate average load
    double total_load = 0.0;
    size_t healthy_cores = 0;
    
    for (const auto& [core_id, metrics] : core_metrics_) {
        if (metrics.is_healthy) {
            total_load += metrics.metrics.cpu_usage;
            healthy_cores++;
        }
    }

    if (healthy_cores == 0) {
        return;
    }

    double avg_load = total_load / healthy_cores;

    // Optimize cores with load significantly different from average
    for (const auto& [core_id, metrics] : core_metrics_) {
        if (metrics.is_healthy && std::abs(metrics.metrics.cpu_usage - avg_load) > 0.2) {
            optimize_core_performance(core_id);
        }
    }
}

void LoadBalancer::monitor_cores() {
    while (running_) {
        {
            std::unique_lock<std::mutex> lock(state_mutex_);
            state_cv_.wait(lock, [this] { return !paused_ || !running_; });
            
            if (!running_) {
                break;
            }
        }

        // Collect metrics from all cores
        if (compute_engine_) {
            auto metrics = compute_engine_->get_system_metrics();
            for (size_t i = 0; i < metrics.size(); ++i) {
                update_core_metrics(i, CoreMetrics{
                    TaskMetrics{
                        metrics[i].cpu_usage,
                        metrics[i].memory_usage,
                        metrics[i].network_usage,
                        metrics[i].queue_size,
                        metrics[i].active_tasks
                    },
                    std::chrono::steady_clock::now(),
                    true
                });
            }
        }

        // Check core health and adjust resources
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        for (const auto& [core_id, metrics] : core_metrics_) {
            if (!metrics.is_healthy) {
                handle_core_failure(core_id);
            } else {
                adjust_resources(core_id);
            }
        }

        // Optimize overall distribution
        optimize_distribution();

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void LoadBalancer::handle_core_failure(size_t core_id) {
    // Mark core as unhealthy
    mark_core_unhealthy(core_id);

    // Redistribute tasks
    redistribute_tasks(core_id);

    // Cleanup
    cleanup_core(core_id);
}

void LoadBalancer::redistribute_tasks(size_t failed_core_id) {
    std::lock_guard<std::mutex> lock(task_mutex_);
    
    // Find tasks assigned to failed core
    std::vector<size_t> failed_tasks;
    for (const auto& [task_id, task] : tasks_) {
        if (task.assigned_core == failed_core_id) {
            failed_tasks.push_back(task_id);
        }
    }

    // Redistribute tasks to healthy cores
    for (size_t task_id : failed_tasks) {
        size_t target_core = find_least_loaded_core();
        if (target_core != static_cast<size_t>(-1)) {
            // Update task assignment
            tasks_[task_id].assigned_core = target_core;
            
            // Resubmit task
            if (tasks_[task_id].type == TaskType::COMPUTE) {
                compute_engine_->submit_task(task_id, tasks_[task_id]);
            } else if (tasks_[task_id].type == TaskType::BLOCKCHAIN) {
                blockchain_engine_->submit_task(task_id, tasks_[task_id]);
            } else if (tasks_[task_id].type == TaskType::NETWORK) {
                network_manager_->submit_task(task_id, tasks_[task_id]);
            } else if (tasks_[task_id].type == TaskType::STORAGE) {
                storage_manager_->submit_task(task_id, tasks_[task_id]);
            }
        }
    }
}

void LoadBalancer::optimize_core_performance(size_t core_id) {
    if (compute_engine_) {
        compute_engine_->optimize_core_performance(core_id);
    }
    if (blockchain_engine_) {
        blockchain_engine_->optimize_core_performance(core_id);
    }
    if (network_manager_) {
        network_manager_->optimize_core_performance(core_id);
    }
    if (storage_manager_) {
        storage_manager_->optimize_core_performance(core_id);
    }
}

void LoadBalancer::adjust_core_resources(size_t core_id) {
    if (compute_engine_) {
        compute_engine_->adjust_resources(core_id);
    }
    if (blockchain_engine_) {
        blockchain_engine_->adjust_resources(core_id);
    }
    if (network_manager_) {
        network_manager_->adjust_resources(core_id);
    }
    if (storage_manager_) {
        storage_manager_->adjust_resources(core_id);
    }
}

void LoadBalancer::cleanup_core(size_t core_id) {
    if (compute_engine_) {
        compute_engine_->cleanup_core(core_id);
    }
    if (blockchain_engine_) {
        blockchain_engine_->cleanup_core(core_id);
    }
    if (network_manager_) {
        network_manager_->cleanup_core(core_id);
    }
    if (storage_manager_) {
        storage_manager_->cleanup_core(core_id);
    }
}

} // namespace core 