#pragma once

#include "CoreEngine.h"
#include "architecture.h"
#include "blockchain/MultiCoreBlockchain.h"
#include "drivers/atomic_ops.h"
#include "drivers/memory_ops.h"
#include "drivers/math_ops.h"
#include "drivers/blockchain_ops.h"
#include "drivers/compute_ops.h"
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace core {

class MultiCoreEngine {
public:
    struct CoreConfig {
        size_t core_id;
        size_t numa_node;
        bool enable_hyperthreading;
        size_t cache_size;
        size_t memory_limit;
        bool enable_simd;
        bool enable_gpu;
        bool enable_fpga;
    };

    struct SystemMetrics {
        std::vector<float> core_utilization;
        std::vector<float> memory_usage;
        std::vector<float> cache_hit_rates;
        std::vector<float> gpu_utilization;
        std::vector<float> fpga_utilization;
        float network_throughput;
        float blockchain_tps;
    };

    explicit MultiCoreEngine(const std::vector<CoreConfig>& configs);
    ~MultiCoreEngine();

    // Core Management
    void initialize();
    void start();
    void stop();
    void pause();
    void resume();

    // Resource Management
    void allocate_resources(size_t core_id, const ResourceRequest& request);
    void release_resources(size_t core_id, const ResourceHandle& handle);
    void rebalance_resources();

    // Task Management
    void submit_task(size_t core_id, const Task& task);
    void cancel_task(size_t core_id, const TaskId& task_id);
    TaskStatus get_task_status(size_t core_id, const TaskId& task_id);

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
    void enable_hardware_acceleration(size_t core_id, bool enable);
    void configure_accelerator(size_t core_id, const AcceleratorConfig& config);
    void calibrate_hardware();

private:
    struct Core {
        std::unique_ptr<CoreEngine> engine;
        std::thread worker;
        std::atomic<bool> running{false};
        std::atomic<bool> paused{false};
        std::mutex mutex;
        std::condition_variable cv;
        TaskQueue task_queue;
        ResourceManager resource_manager;
        CacheManager cache_manager;
        AcceleratorManager accelerator_manager;
    };

    // Core Management
    std::vector<Core> cores_;
    std::vector<CoreConfig> configs_;
    std::atomic<bool> system_running_{false};
    std::mutex system_mutex_;
    std::condition_variable system_cv_;

    // Blockchain
    std::unique_ptr<MultiCoreBlockchain> blockchain_;
    std::mutex blockchain_mutex_;

    // Monitoring
    std::atomic<bool> monitoring_enabled_{false};
    std::function<void(const SystemMetrics&)> metrics_callback_;
    std::thread monitoring_thread_;

    // Internal Methods
    void initialize_core(size_t core_id);
    void worker_thread(size_t core_id);
    void monitor_system();
    void handle_core_failure(size_t core_id);
    void redistribute_tasks();
    void update_metrics();
    void cleanup_core(size_t core_id);
    void sync_cores();
    void optimize_core_performance(size_t core_id);
    void configure_core_affinity(size_t core_id);
    void setup_inter_core_communication();
    void initialize_hardware_acceleration(size_t core_id);
    void calibrate_core_hardware(size_t core_id);
    void verify_core_integrity(size_t core_id);
    void backup_core_state(size_t core_id);
    void restore_core_state(size_t core_id);
    void handle_core_exception(size_t core_id, const std::exception& e);
    void log_core_metrics(size_t core_id);
    void adjust_core_resources(size_t core_id);
    void optimize_core_cache(size_t core_id);
    void configure_core_memory(size_t core_id);
    void setup_core_security(size_t core_id);
    void initialize_core_blockchain(size_t core_id);
    void sync_core_blockchain(size_t core_id);
    void verify_core_blockchain(size_t core_id);
    void optimize_core_blockchain(size_t core_id);
};

} // namespace core 