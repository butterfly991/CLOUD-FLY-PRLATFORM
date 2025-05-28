#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <chrono>
#include <mutex>
#include <atomic>
#include <thread>

namespace core {
namespace storage {

enum class StorageType {
    BLOCK,
    FILE,
    OBJECT
};

enum class ReplicationStrategy {
    SYNCHRONOUS,
    ASYNCHRONOUS,
    NONE
};

struct StorageConfig {
    std::string name;
    StorageType type;
    std::string path;
    size_t size_mb;
    ReplicationStrategy replication;
    uint32_t replication_factor;
    bool encryption_enabled;
    std::string encryption_key;
    bool compression_enabled;
    std::string compression_algorithm;
};

struct StorageStats {
    size_t total_size;
    size_t used_size;
    size_t available_size;
    uint64_t read_operations;
    uint64_t write_operations;
    double read_latency_ms;
    double write_latency_ms;
    std::chrono::system_clock::time_point last_updated;
};

class StorageManager {
public:
    static StorageManager& get_instance() {
        static StorageManager instance;
        return instance;
    }

    // Storage Management
    void create_storage(const StorageConfig& config);
    void delete_storage(const std::string& name);
    void update_storage(const std::string& name, const StorageConfig& new_config);
    std::vector<StorageConfig> list_storages() const;
    
    // Data Operations
    void write_data(const std::string& storage_name,
                   const std::string& key,
                   const std::vector<uint8_t>& data);
    std::vector<uint8_t> read_data(const std::string& storage_name,
                                 const std::string& key);
    void delete_data(const std::string& storage_name,
                    const std::string& key);
    bool exists(const std::string& storage_name,
               const std::string& key);
    
    // Replication Management
    void start_replication(const std::string& storage_name);
    void stop_replication(const std::string& storage_name);
    void update_replication_strategy(const std::string& storage_name,
                                   ReplicationStrategy strategy);
    
    // Backup Management
    void create_backup(const std::string& storage_name,
                      const std::string& backup_name);
    void restore_from_backup(const std::string& storage_name,
                           const std::string& backup_name);
    void delete_backup(const std::string& storage_name,
                      const std::string& backup_name);
    std::vector<std::string> list_backups(const std::string& storage_name) const;
    
    // Monitoring
    void start_storage_monitoring();
    void stop_storage_monitoring();
    StorageStats get_storage_stats(const std::string& storage_name) const;
    
    // Encryption Management
    void enable_encryption(const std::string& storage_name,
                         const std::string& key);
    void disable_encryption(const std::string& storage_name);
    void rotate_encryption_key(const std::string& storage_name,
                             const std::string& new_key);
    
    // Compression Management
    void enable_compression(const std::string& storage_name,
                          const std::string& algorithm);
    void disable_compression(const std::string& storage_name);
    
    // Snapshot Management
    void create_snapshot(const std::string& storage_name,
                        const std::string& snapshot_name);
    void restore_from_snapshot(const std::string& storage_name,
                             const std::string& snapshot_name);
    void delete_snapshot(const std::string& storage_name,
                        const std::string& snapshot_name);
    std::vector<std::string> list_snapshots(const std::string& storage_name) const;

private:
    StorageManager() = default;
    ~StorageManager() = default;
    
    StorageManager(const StorageManager&) = delete;
    StorageManager& operator=(const StorageManager&) = delete;

    struct Storage {
        StorageConfig config;
        StorageStats stats;
        std::chrono::system_clock::time_point created_at;
        bool is_active;
        std::unordered_map<std::string, std::vector<uint8_t>> data;
        std::vector<std::string> backups;
        std::vector<std::string> snapshots;
    };

    std::unordered_map<std::string, Storage> storages_;
    mutable std::mutex storages_mutex_;
    
    std::atomic<bool> monitoring_active_{false};
    std::thread monitoring_thread_;
    
    void monitoring_worker();
    void update_storage_stats();
    void check_storage_health();
    void cleanup_inactive_storages();
    
    // Helper functions
    bool validate_storage_config(const StorageConfig& config) const;
    void apply_storage_changes(const Storage& storage);
    void replicate_data(const std::string& storage_name,
                       const std::string& key,
                       const std::vector<uint8_t>& data);
    void encrypt_data(std::vector<uint8_t>& data,
                     const std::string& key);
    void decrypt_data(std::vector<uint8_t>& data,
                     const std::string& key);
    void compress_data(std::vector<uint8_t>& data,
                      const std::string& algorithm);
    void decompress_data(std::vector<uint8_t>& data,
                        const std::string& algorithm);
};

} // namespace storage
} // namespace core 