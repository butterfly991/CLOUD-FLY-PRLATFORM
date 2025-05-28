#include "blockchain/MultiCoreBlockchain.h"
#include "drivers/blockchain_ops.h"
#include "drivers/atomic_ops.h"
#include "drivers/memory_ops.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>

namespace core {
namespace blockchain {

MultiCoreBlockchain::MultiCoreBlockchain(size_t num_cores)
    : num_cores_(num_cores) {
    cores_.resize(num_cores);
    core_metrics_.resize(num_cores);
}

MultiCoreBlockchain::~MultiCoreBlockchain() {
    stop();
}

void MultiCoreBlockchain::initialize() {
    global_state_ = std::make_unique<StateManager>();
    consensus_ = std::make_unique<ConsensusManager>();

    for (size_t i = 0; i < num_cores_; ++i) {
        initialize_core(i);
    }
}

void MultiCoreBlockchain::start() {
    if (running_.exchange(true)) {
        return;
    }

    for (size_t i = 0; i < num_cores_; ++i) {
        cores_[i].running = true;
        cores_[i].worker = std::thread(&MultiCoreBlockchain::worker_thread, this, i);
    }
}

void MultiCoreBlockchain::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    for (auto& core : cores_) {
        core.running = false;
        core.cv.notify_all();
        if (core.worker.joinable()) {
            core.worker.join();
        }
    }
}

void MultiCoreBlockchain::initialize_core(size_t core_id) {
    auto& core = cores_[core_id];

    // Initialize blockchain engine
    core.engine = std::make_unique<BlockchainEngine>();
    core.engine->initialize();

    // Initialize state manager
    core.state_manager.initialize();

    // Initialize consensus manager
    core.consensus_manager.initialize();

    // Setup queues
    core.tx_queue.initialize();
    core.block_queue.initialize();
}

void MultiCoreBlockchain::worker_thread(size_t core_id) {
    auto& core = cores_[core_id];
    
    while (core.running) {
        std::unique_lock<std::mutex> lock(core.mutex);
        core.cv.wait(lock, [&core] { 
            return !core.running || (!core.paused && 
                   (!core.tx_queue.empty() || !core.block_queue.empty())); 
        });

        if (!core.running) {
            break;
        }

        if (core.paused) {
            continue;
        }

        // Process transactions
        process_core_transactions(core_id);

        // Validate blocks
        validate_core_blocks(core_id);

        // Sync state
        sync_core_state(core_id);

        // Optimize performance
        optimize_core_performance(core_id);
    }
}

void MultiCoreBlockchain::process_core_transactions(size_t core_id) {
    auto& core = cores_[core_id];
    
    while (!core.tx_queue.empty()) {
        auto tx = core.tx_queue.pop();
        
        try {
            // Validate transaction
            if (!core.engine->validate_transaction(*tx)) {
                continue;
            }

            // Process transaction
            core.engine->process_transaction(*tx);

            // Update metrics
            core_metrics_[core_id].transaction_throughput++;
            
        } catch (const std::exception& e) {
            handle_core_failure(core_id);
            break;
        }
    }
}

void MultiCoreBlockchain::validate_core_blocks(size_t core_id) {
    auto& core = cores_[core_id];
    
    while (!core.block_queue.empty()) {
        auto block = core.block_queue.pop();
        
        try {
            // Validate block
            if (!core.engine->validate_block(*block)) {
                continue;
            }

            // Commit block
            core.engine->commit_block(*block);

            // Update metrics
            BlockMetrics metrics;
            metrics.transactions_count = block->transactions.size();
            metrics.block_size = block->size;
            metrics.processing_time = block->processing_time;
            metrics.validation_time = block->validation_time;
            metrics.consensus_time = block->consensus_time;
            
            block_metrics_[block->id] = metrics;
            
        } catch (const std::exception& e) {
            handle_core_failure(core_id);
            break;
        }
    }
}

void MultiCoreBlockchain::sync_core_state(size_t core_id) {
    auto& core = cores_[core_id];
    
    std::lock_guard<std::mutex> state_lock(state_mutex_);
    
    // Sync with global state
    core.state_manager.sync_with_global(*global_state_);
    
    // Verify state integrity
    if (!core.state_manager.verify_integrity()) {
        handle_core_failure(core_id);
        return;
    }
    
    // Optimize state
    core.state_manager.optimize();
}

void MultiCoreBlockchain::optimize_core_performance(size_t core_id) {
    auto& core = cores_[core_id];
    
    // Optimize memory
    optimize_core_memory(core_id);
    
    // Optimize validation
    optimize_core_validation(core_id);
    
    // Optimize transactions
    optimize_core_transactions(core_id);
    
    // Calibrate consensus
    calibrate_core_consensus(core_id);
}

void MultiCoreBlockchain::optimize_core_memory(size_t core_id) {
    auto& core = cores_[core_id];
    
    // Compact memory if usage is high
    if (core_metrics_[core_id].memory_usage > 0.8) {
        core.state_manager.compact();
    }
    
    // Optimize cache usage
    if (core_metrics_[core_id].cache_hits < 0.7) {
        core.engine->optimize_cache();
    }
}

void MultiCoreBlockchain::optimize_core_validation(size_t core_id) {
    auto& core = cores_[core_id];
    
    // Adjust validation parameters based on metrics
    if (core_metrics_[core_id].validation_speed < 0.5) {
        core.engine->increase_validation_parallelism();
    }
    
    // Verify validation integrity
    verify_core_validation(core_id);
}

void MultiCoreBlockchain::optimize_core_transactions(size_t core_id) {
    auto& core = cores_[core_id];
    
    // Adjust transaction processing parameters
    if (core_metrics_[core_id].transaction_throughput < 0.5) {
        core.engine->increase_transaction_parallelism();
    }
    
    // Verify transaction processing
    verify_core_transactions(core_id);
}

void MultiCoreBlockchain::calibrate_core_consensus(size_t core_id) {
    auto& core = cores_[core_id];
    
    // Adjust consensus parameters
    if (core_metrics_[core_id].consensus_participation < 0.7) {
        core.consensus_manager.adjust_parameters();
    }
    
    // Verify consensus
    verify_core_consensus(core_id);
}

void MultiCoreBlockchain::handle_core_failure(size_t core_id) {
    auto& core = cores_[core_id];
    
    // Backup state
    backup_core_state(core_id);
    
    // Attempt recovery
    try {
        core.engine->emergency_stop();
        core.state_manager.rollback();
        core.consensus_manager.reset();
        
        // Reinitialize core
        initialize_core(core_id);
        
        // Restore state
        restore_core_state(core_id);
        
    } catch (const std::exception& e) {
        // If recovery fails, redistribute work
        redistribute_work();
    }
}

void MultiCoreBlockchain::redistribute_work() {
    std::vector<Transaction> failed_txs;
    std::vector<Block> failed_blocks;
    
    // Collect work from failed cores
    for (size_t i = 0; i < num_cores_; ++i) {
        if (!cores_[i].running) {
            auto txs = cores_[i].tx_queue.drain();
            auto blocks = cores_[i].block_queue.drain();
            
            failed_txs.insert(failed_txs.end(), txs.begin(), txs.end());
            failed_blocks.insert(failed_blocks.end(), blocks.begin(), blocks.end());
        }
    }
    
    // Redistribute to healthy cores
    for (const auto& tx : failed_txs) {
        size_t target_core = find_least_loaded_core();
        if (target_core != static_cast<size_t>(-1)) {
            cores_[target_core].tx_queue.push(tx);
        }
    }
    
    for (const auto& block : failed_blocks) {
        size_t target_core = find_least_loaded_core();
        if (target_core != static_cast<size_t>(-1)) {
            cores_[target_core].block_queue.push(block);
        }
    }
}

size_t MultiCoreBlockchain::find_least_loaded_core() const {
    size_t min_load = std::numeric_limits<size_t>::max();
    size_t target_core = static_cast<size_t>(-1);
    
    for (size_t i = 0; i < num_cores_; ++i) {
        if (!cores_[i].running) continue;
        
        auto load = cores_[i].tx_queue.size() + cores_[i].block_queue.size();
        if (load < min_load) {
            min_load = load;
            target_core = i;
        }
    }
    
    return target_core;
}

void MultiCoreBlockchain::update_metrics() {
    float total_tps = 0.0f;
    
    for (size_t i = 0; i < num_cores_; ++i) {
        total_tps += core_metrics_[i].transaction_throughput;
    }
    
    tps_.store(total_tps);
}

float MultiCoreBlockchain::get_transactions_per_second() const {
    return tps_.load();
}

MultiCoreBlockchain::BlockMetrics MultiCoreBlockchain::get_block_metrics(
    const BlockId& block_id) const {
    auto it = block_metrics_.find(block_id);
    if (it != block_metrics_.end()) {
        return it->second;
    }
    return BlockMetrics{};
}

MultiCoreBlockchain::CoreMetrics MultiCoreBlockchain::get_core_metrics(
    size_t core_id) const {
    if (core_id < core_metrics_.size()) {
        return core_metrics_[core_id];
    }
    return CoreMetrics{};
}

} // namespace blockchain
} // namespace core 