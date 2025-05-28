#pragma once

#include "blockchain_ops.h"
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <unordered_map>

namespace core {
namespace blockchain {

class MultiCoreBlockchain {
public:
    struct BlockMetrics {
        size_t transactions_count;
        size_t block_size;
        float processing_time;
        float validation_time;
        float consensus_time;
    };

    struct CoreMetrics {
        float transaction_throughput;
        float validation_speed;
        float consensus_participation;
        size_t memory_usage;
        size_t cache_hits;
    };

    explicit MultiCoreBlockchain(size_t num_cores);
    ~MultiCoreBlockchain();

    // Core Operations
    void initialize();
    void start();
    void stop();
    void pause();
    void resume();

    // Transaction Management
    bool validate_transaction(const Transaction& tx);
    void process_transaction(const Transaction& tx);
    void broadcast_transaction(const Transaction& tx);
    TransactionStatus get_transaction_status(const TransactionId& tx_id);

    // Block Management
    void create_block();
    void validate_block(const Block& block);
    void commit_block(const Block& block);
    void rollback_block(const BlockId& block_id);

    // Consensus
    void participate_consensus();
    void verify_consensus();
    void sync_with_network();

    // State Management
    bool verify_state() const;
    void backup_state();
    void restore_state();
    void compact_state();

    // Performance
    float get_transactions_per_second() const;
    BlockMetrics get_block_metrics(const BlockId& block_id) const;
    CoreMetrics get_core_metrics(size_t core_id) const;
    void optimize_performance();

private:
    struct Core {
        std::unique_ptr<BlockchainEngine> engine;
        std::thread worker;
        std::atomic<bool> running{false};
        std::atomic<bool> paused{false};
        std::mutex mutex;
        std::condition_variable cv;
        TransactionQueue tx_queue;
        BlockQueue block_queue;
        StateManager state_manager;
        ConsensusManager consensus_manager;
    };

    // Core Management
    std::vector<Core> cores_;
    size_t num_cores_;
    std::atomic<bool> running_{false};
    std::mutex system_mutex_;
    std::condition_variable system_cv_;

    // State
    std::unique_ptr<StateManager> global_state_;
    std::mutex state_mutex_;

    // Consensus
    std::unique_ptr<ConsensusManager> consensus_;
    std::mutex consensus_mutex_;

    // Metrics
    std::atomic<float> tps_{0.0f};
    std::vector<CoreMetrics> core_metrics_;
    std::unordered_map<BlockId, BlockMetrics> block_metrics_;

    // Internal Methods
    void initialize_core(size_t core_id);
    void worker_thread(size_t core_id);
    void process_core_transactions(size_t core_id);
    void validate_core_blocks(size_t core_id);
    void sync_core_state(size_t core_id);
    void optimize_core_performance(size_t core_id);
    void handle_core_failure(size_t core_id);
    void redistribute_work();
    void update_metrics();
    void verify_core_integrity(size_t core_id);
    void backup_core_state(size_t core_id);
    void restore_core_state(size_t core_id);
    void compact_core_state(size_t core_id);
    void calibrate_core_consensus(size_t core_id);
    void optimize_core_memory(size_t core_id);
    void verify_core_consensus(size_t core_id);
    void sync_core_consensus(size_t core_id);
    void optimize_core_validation(size_t core_id);
    void verify_core_validation(size_t core_id);
    void optimize_core_transactions(size_t core_id);
    void verify_core_transactions(size_t core_id);
};

} // namespace blockchain
} // namespace core 