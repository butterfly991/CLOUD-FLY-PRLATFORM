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
namespace container {

enum class ContainerState {
    CREATED,
    RUNNING,
    PAUSED,
    STOPPED,
    ERROR
};

enum class ResourceType {
    CPU,
    MEMORY,
    DISK,
    NETWORK
};

struct ResourceLimits {
    double cpu_limit;
    size_t memory_limit_mb;
    size_t disk_limit_mb;
    size_t network_bandwidth_mbps;
};

struct ContainerConfig {
    std::string image;
    std::string name;
    std::vector<std::string> command;
    std::vector<std::string> environment;
    std::unordered_map<std::string, std::string> volumes;
    ResourceLimits resource_limits;
    bool auto_restart;
    std::chrono::seconds health_check_interval;
    std::string network_mode;
};

struct ContainerStats {
    double cpu_usage;
    size_t memory_usage;
    size_t disk_usage;
    size_t network_io;
    ContainerState state;
    std::chrono::system_clock::time_point last_updated;
};

class ContainerManager {
public:
    static ContainerManager& get_instance() {
        static ContainerManager instance;
        return instance;
    }

    // Container Lifecycle
    std::string create_container(const ContainerConfig& config);
    void start_container(const std::string& container_id);
    void stop_container(const std::string& container_id);
    void remove_container(const std::string& container_id);
    void pause_container(const std::string& container_id);
    void resume_container(const std::string& container_id);
    
    // Container Management
    ContainerState get_container_state(const std::string& container_id) const;
    ContainerStats get_container_stats(const std::string& container_id) const;
    std::vector<std::string> list_containers() const;
    void update_container_resources(const std::string& container_id,
                                  const ResourceLimits& new_limits);
    
    // Resource Management
    void set_resource_limits(const std::string& container_id,
                           ResourceType type,
                           double limit);
    double get_resource_usage(const std::string& container_id,
                            ResourceType type) const;
    
    // Health Monitoring
    void start_health_monitoring();
    void stop_health_monitoring();
    bool is_container_healthy(const std::string& container_id) const;
    
    // Networking
    void configure_network(const std::string& container_id,
                          const std::string& network_config);
    void expose_port(const std::string& container_id,
                    uint16_t host_port,
                    uint16_t container_port);
    
    // Volume Management
    void mount_volume(const std::string& container_id,
                     const std::string& host_path,
                     const std::string& container_path);
    void unmount_volume(const std::string& container_id,
                       const std::string& container_path);

private:
    ContainerManager() = default;
    ~ContainerManager() = default;
    
    ContainerManager(const ContainerManager&) = delete;
    ContainerManager& operator=(const ContainerManager&) = delete;

    struct Container {
        ContainerConfig config;
        ContainerStats stats;
        std::string id;
        std::chrono::system_clock::time_point created_at;
        std::chrono::system_clock::time_point last_health_check;
        bool is_healthy;
    };

    std::unordered_map<std::string, Container> containers_;
    mutable std::mutex containers_mutex_;
    std::atomic<bool> monitoring_active_{false};
    std::thread monitoring_thread_;
    
    void monitoring_worker();
    void update_container_stats(Container& container);
    bool check_container_health(Container& container);
    void cleanup_stopped_containers();
    std::string generate_container_id() const;
    
    // Docker/Kubernetes integration
    void initialize_docker_client();
    void initialize_kubernetes_client();
    void sync_with_orchestrator();
};

} // namespace container
} // namespace core 