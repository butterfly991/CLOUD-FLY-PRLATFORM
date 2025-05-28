#pragma once

#include "MultiCoreEngine.h"
#include "blockchain/MultiCoreBlockchain.h"
#include "load_balancer/LoadBalancer.h"
#include "monitoring/MonitoringSystem.h"
#include "network/NetworkManager.h"
#include "storage/StorageManager.h"
#include "container/ContainerManager.h"
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace core {

class ParentCore {
public:
    struct SystemConfig {
        size_t num_cores;
        size_t num_blockchain_cores;
        size_t num_compute_cores;
        size_t num_network_cores;
        size_t num_storage_cores;
        bool enable_gpu;
        bool enable_fpga;
        bool enable_smartnic;
        size_t memory_limit;
        size_t cache_size;
    };

    struct SystemState {
        std::atomic<bool> running{false};
        std::atomic<bool> initialized{false};
        std::atomic<bool> paused{false};
        std::mutex state_mutex;
        std::condition_variable state_cv;
    };

    explicit ParentCore(const SystemConfig& config);
    ~ParentCore();

    // Core Management
    void initialize();
    void start();
    void stop();
    void pause();
    void resume();

    // Resource Management
    void allocate_resources(const ResourceRequest& request);
    void release_resources(const ResourceHandle& handle);
    void rebalance_resources();

    // Task Management
    void submit_task(const Task& task);
    void cancel_task(const TaskId& task_id);
    TaskStatus get_task_status(const TaskId& task_id);

    // Blockchain Operations
    void initialize_blockchain();
    void process_blockchain_transaction(const Transaction& tx);
    void verify_blockchain_integrity();
    void sync_blockchain_state();

    // Monitoring and Metrics
    SystemMetrics get_system_metrics() const;
    void enable_monitoring(bool enable);
    void set_metrics_callback(std::function<void(const SystemMetrics&)> callback);

    // Hardware Acceleration
    void enable_hardware_acceleration(bool enable);
    void configure_accelerator(const AcceleratorConfig& config);
    void calibrate_hardware();

private:
    // Core Components
    std::unique_ptr<MultiCoreEngine> compute_engine_;
    std::unique_ptr<MultiCoreBlockchain> blockchain_engine_;
    std::unique_ptr<LoadBalancer> load_balancer_;
    std::unique_ptr<MonitoringSystem> monitoring_system_;
    std::unique_ptr<NetworkManager> network_manager_;
    std::unique_ptr<StorageManager> storage_manager_;
    std::unique_ptr<ContainerManager> container_manager_;

    // System State
    SystemConfig config_;
    SystemState state_;

    // Internal Methods
    void initialize_cores();
    void initialize_blockchain_cores();
    void initialize_compute_cores();
    void initialize_network_cores();
    void initialize_storage_cores();
    void setup_inter_core_communication();
    void monitor_system();
    void handle_core_failure(size_t core_id);
    void redistribute_tasks();
    void update_metrics();
    void cleanup_cores();
    void sync_cores();
    void optimize_core_performance(size_t core_id);
    void configure_core_affinity(size_t core_id);
    void setup_core_security(size_t core_id);
    void initialize_core_blockchain(size_t core_id);
    void sync_core_blockchain(size_t core_id);
    void verify_core_blockchain(size_t core_id);
    void optimize_core_blockchain(size_t core_id);
    void handle_core_exception(size_t core_id, const std::exception& e);
    void log_core_metrics(size_t core_id);
    void adjust_core_resources(size_t core_id);
    void optimize_core_cache(size_t core_id);
    void configure_core_memory(size_t core_id);
    void backup_core_state(size_t core_id);
    void restore_core_state(size_t core_id);
};

} // namespace core 