#include "storage_manager.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <zlib.h>
#include <lz4.h>
#include <snappy.h>

namespace core {
namespace storage {

void StorageManager::create_storage(const StorageConfig& config) {
    if (!validate_storage_config(config)) {
        throw std::invalid_argument("Invalid storage configuration");
    }
    
    std::lock_guard<std::mutex> lock(storages_mutex_);
    Storage storage;
    storage.config = config;
    storage.created_at = std::chrono::system_clock::now();
    storage.is_active = true;
    
    // Initialize storage statistics
    storage.stats.total_size = config.size_mb * 1024 * 1024;
    storage.stats.used_size = 0;
    storage.stats.available_size = storage.stats.total_size;
    storage.stats.read_operations = 0;
    storage.stats.write_operations = 0;
    storage.stats.read_latency_ms = 0.0;
    storage.stats.write_latency_ms = 0.0;
    storage.stats.last_updated = storage.created_at;
    
    storages_[config.name] = std::move(storage);
    apply_storage_changes(storages_[config.name]);
}

void StorageManager::delete_storage(const std::string& name) {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    if (auto it = storages_.find(name); it != storages_.end()) {
        it->second.is_active = false;
        storages_.erase(it);
    }
}

void StorageManager::update_storage(const std::string& name,
                                 const StorageConfig& new_config) {
    if (!validate_storage_config(new_config)) {
        throw std::invalid_argument("Invalid storage configuration");
    }
    
    std::lock_guard<std::mutex> lock(storages_mutex_);
    if (auto it = storages_.find(name); it != storages_.end()) {
        it->second.config = new_config;
        apply_storage_changes(it->second);
    }
}

std::vector<StorageConfig> StorageManager::list_storages() const {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    std::vector<StorageConfig> result;
    result.reserve(storages_.size());
    
    for (const auto& [name, storage] : storages_) {
        result.push_back(storage.config);
    }
    
    return result;
}

void StorageManager::write_data(const std::string& storage_name,
                              const std::string& key,
                              const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    if (auto it = storages_.find(storage_name); it != storages_.end()) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::vector<uint8_t> processed_data = data;
        
        // Apply compression if enabled
        if (it->second.config.compression_enabled) {
            compress_data(processed_data, it->second.config.compression_algorithm);
        }
        
        // Apply encryption if enabled
        if (it->second.config.encryption_enabled) {
            encrypt_data(processed_data, it->second.config.encryption_key);
        }
        
        // Store the data
        it->second.data[key] = std::move(processed_data);
        
        // Update statistics
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        it->second.stats.write_latency_ms = duration.count();
        it->second.stats.write_operations++;
        it->second.stats.used_size += processed_data.size();
        it->second.stats.available_size = it->second.stats.total_size - it->second.stats.used_size;
        it->second.stats.last_updated = std::chrono::system_clock::now();
        
        // Replicate data if replication is enabled
        if (it->second.config.replication != ReplicationStrategy::NONE) {
            replicate_data(storage_name, key, processed_data);
        }
    }
}

std::vector<uint8_t> StorageManager::read_data(const std::string& storage_name,
                                            const std::string& key) {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    if (auto it = storages_.find(storage_name); it != storages_.end()) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        if (auto data_it = it->second.data.find(key); data_it != it->second.data.end()) {
            std::vector<uint8_t> processed_data = data_it->second;
            
            // Decrypt data if encryption is enabled
            if (it->second.config.encryption_enabled) {
                decrypt_data(processed_data, it->second.config.encryption_key);
            }
            
            // Decompress data if compression is enabled
            if (it->second.config.compression_enabled) {
                decompress_data(processed_data, it->second.config.compression_algorithm);
            }
            
            // Update statistics
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            it->second.stats.read_latency_ms = duration.count();
            it->second.stats.read_operations++;
            it->second.stats.last_updated = std::chrono::system_clock::now();
            
            return processed_data;
        }
    }
    return {};
}

void StorageManager::delete_data(const std::string& storage_name,
                               const std::string& key) {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    if (auto it = storages_.find(storage_name); it != storages_.end()) {
        if (auto data_it = it->second.data.find(key); data_it != it->second.data.end()) {
            it->second.stats.used_size -= data_it->second.size();
            it->second.stats.available_size = it->second.stats.total_size - it->second.stats.used_size;
            it->second.data.erase(data_it);
            it->second.stats.last_updated = std::chrono::system_clock::now();
        }
    }
}

bool StorageManager::exists(const std::string& storage_name,
                          const std::string& key) {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    if (auto it = storages_.find(storage_name); it != storages_.end()) {
        return it->second.data.find(key) != it->second.data.end();
    }
    return false;
}

void StorageManager::start_replication(const std::string& storage_name) {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    if (auto it = storages_.find(storage_name); it != storages_.end()) {
        // Implementation would depend on the replication system
        // This is a placeholder for the actual implementation
    }
}

void StorageManager::stop_replication(const std::string& storage_name) {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    if (auto it = storages_.find(storage_name); it != storages_.end()) {
        // Implementation would depend on the replication system
        // This is a placeholder for the actual implementation
    }
}

void StorageManager::update_replication_strategy(const std::string& storage_name,
                                              ReplicationStrategy strategy) {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    if (auto it = storages_.find(storage_name); it != storages_.end()) {
        it->second.config.replication = strategy;
        apply_storage_changes(it->second);
    }
}

void StorageManager::create_backup(const std::string& storage_name,
                                const std::string& backup_name) {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    if (auto it = storages_.find(storage_name); it != storages_.end()) {
        // Implementation would depend on the backup system
        // This is a placeholder for the actual implementation
        it->second.backups.push_back(backup_name);
    }
}

void StorageManager::restore_from_backup(const std::string& storage_name,
                                     const std::string& backup_name) {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    if (auto it = storages_.find(storage_name); it != storages_.end()) {
        // Implementation would depend on the backup system
        // This is a placeholder for the actual implementation
    }
}

void StorageManager::delete_backup(const std::string& storage_name,
                                const std::string& backup_name) {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    if (auto it = storages_.find(storage_name); it != storages_.end()) {
        it->second.backups.erase(
            std::remove(it->second.backups.begin(),
                       it->second.backups.end(),
                       backup_name),
            it->second.backups.end()
        );
    }
}

std::vector<std::string> StorageManager::list_backups(const std::string& storage_name) const {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    if (auto it = storages_.find(storage_name); it != storages_.end()) {
        return it->second.backups;
    }
    return {};
}

void StorageManager::start_storage_monitoring() {
    if (monitoring_active_.exchange(true)) {
        return;
    }
    
    monitoring_thread_ = std::thread([this]() {
        monitoring_worker();
    });
}

void StorageManager::stop_storage_monitoring() {
    monitoring_active_ = false;
    if (monitoring_thread_.joinable()) {
        monitoring_thread_.join();
    }
}

StorageStats StorageManager::get_storage_stats(const std::string& storage_name) const {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    if (auto it = storages_.find(storage_name); it != storages_.end()) {
        return it->second.stats;
    }
    return StorageStats{};
}

void StorageManager::enable_encryption(const std::string& storage_name,
                                    const std::string& key) {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    if (auto it = storages_.find(storage_name); it != storages_.end()) {
        it->second.config.encryption_enabled = true;
        it->second.config.encryption_key = key;
        apply_storage_changes(it->second);
    }
}

void StorageManager::disable_encryption(const std::string& storage_name) {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    if (auto it = storages_.find(storage_name); it != storages_.end()) {
        it->second.config.encryption_enabled = false;
        apply_storage_changes(it->second);
    }
}

void StorageManager::rotate_encryption_key(const std::string& storage_name,
                                        const std::string& new_key) {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    if (auto it = storages_.find(storage_name); it != storages_.end()) {
        it->second.config.encryption_key = new_key;
        apply_storage_changes(it->second);
    }
}

void StorageManager::enable_compression(const std::string& storage_name,
                                     const std::string& algorithm) {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    if (auto it = storages_.find(storage_name); it != storages_.end()) {
        it->second.config.compression_enabled = true;
        it->second.config.compression_algorithm = algorithm;
        apply_storage_changes(it->second);
    }
}

void StorageManager::disable_compression(const std::string& storage_name) {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    if (auto it = storages_.find(storage_name); it != storages_.end()) {
        it->second.config.compression_enabled = false;
        apply_storage_changes(it->second);
    }
}

void StorageManager::create_snapshot(const std::string& storage_name,
                                  const std::string& snapshot_name) {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    if (auto it = storages_.find(storage_name); it != storages_.end()) {
        // Implementation would depend on the snapshot system
        // This is a placeholder for the actual implementation
        it->second.snapshots.push_back(snapshot_name);
    }
}

void StorageManager::restore_from_snapshot(const std::string& storage_name,
                                       const std::string& snapshot_name) {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    if (auto it = storages_.find(storage_name); it != storages_.end()) {
        // Implementation would depend on the snapshot system
        // This is a placeholder for the actual implementation
    }
}

void StorageManager::delete_snapshot(const std::string& storage_name,
                                  const std::string& snapshot_name) {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    if (auto it = storages_.find(storage_name); it != storages_.end()) {
        it->second.snapshots.erase(
            std::remove(it->second.snapshots.begin(),
                       it->second.snapshots.end(),
                       snapshot_name),
            it->second.snapshots.end()
        );
    }
}

std::vector<std::string> StorageManager::list_snapshots(const std::string& storage_name) const {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    if (auto it = storages_.find(storage_name); it != storages_.end()) {
        return it->second.snapshots;
    }
    return {};
}

void StorageManager::monitoring_worker() {
    while (monitoring_active_) {
        update_storage_stats();
        check_storage_health();
        cleanup_inactive_storages();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void StorageManager::update_storage_stats() {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    for (auto& [name, storage] : storages_) {
        // Update storage statistics
        // This is a placeholder for the actual implementation
        storage.stats.last_updated = std::chrono::system_clock::now();
    }
}

void StorageManager::check_storage_health() {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    for (auto& [name, storage] : storages_) {
        // Check storage health
        // This is a placeholder for the actual implementation
    }
}

void StorageManager::cleanup_inactive_storages() {
    std::lock_guard<std::mutex> lock(storages_mutex_);
    for (auto it = storages_.begin(); it != storages_.end();) {
        if (!it->second.is_active) {
            it = storages_.erase(it);
        } else {
            ++it;
        }
    }
}

bool StorageManager::validate_storage_config(const StorageConfig& config) const {
    // Validate storage configuration
    // This is a placeholder for the actual implementation
    return true;
}

void StorageManager::apply_storage_changes(const Storage& storage) {
    // Apply storage configuration changes
    // This is a placeholder for the actual implementation
}

void StorageManager::replicate_data(const std::string& storage_name,
                                 const std::string& key,
                                 const std::vector<uint8_t>& data) {
    // Implementation would depend on the replication system
    // This is a placeholder for the actual implementation
}

void StorageManager::encrypt_data(std::vector<uint8_t>& data,
                               const std::string& key) {
    // Implementation would depend on the encryption system
    // This is a placeholder for the actual implementation
}

void StorageManager::decrypt_data(std::vector<uint8_t>& data,
                               const std::string& key) {
    // Implementation would depend on the encryption system
    // This is a placeholder for the actual implementation
}

void StorageManager::compress_data(std::vector<uint8_t>& data,
                                const std::string& algorithm) {
    // Implementation would depend on the compression system
    // This is a placeholder for the actual implementation
}

void StorageManager::decompress_data(std::vector<uint8_t>& data,
                                  const std::string& algorithm) {
    // Implementation would depend on the compression system
    // This is a placeholder for the actual implementation
}

} // namespace storage
} // namespace core 