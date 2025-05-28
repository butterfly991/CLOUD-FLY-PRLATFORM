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
namespace network {

enum class NetworkType {
    VIRTUAL,
    PHYSICAL,
    OVERLAY
};

enum class Protocol {
    TCP,
    UDP,
    ICMP,
    HTTP,
    HTTPS,
    GRPC
};

struct NetworkConfig {
    std::string name;
    NetworkType type;
    std::string subnet;
    std::string gateway;
    std::vector<std::string> dns_servers;
    bool enable_dhcp;
    bool enable_nat;
    std::string mtu;
};

struct SecurityGroup {
    std::string name;
    std::vector<std::string> allowed_ports;
    std::vector<std::string> allowed_protocols;
    std::vector<std::string> allowed_ips;
    bool enable_logging;
};

struct LoadBalancerConfig {
    std::string name;
    std::string algorithm;
    std::vector<std::string> backend_servers;
    uint16_t port;
    Protocol protocol;
    bool enable_ssl;
    std::string ssl_cert;
    std::string ssl_key;
};

class NetworkManager {
public:
    static NetworkManager& get_instance() {
        static NetworkManager instance;
        return instance;
    }

    // Network Management
    void create_network(const NetworkConfig& config);
    void delete_network(const std::string& name);
    void update_network(const std::string& name, const NetworkConfig& new_config);
    std::vector<NetworkConfig> list_networks() const;
    
    // Security Management
    void create_security_group(const SecurityGroup& group);
    void update_security_group(const std::string& name, const SecurityGroup& new_group);
    void delete_security_group(const std::string& name);
    std::vector<SecurityGroup> list_security_groups() const;
    
    // Load Balancer Management
    void create_load_balancer(const LoadBalancerConfig& config);
    void update_load_balancer(const std::string& name, const LoadBalancerConfig& new_config);
    void delete_load_balancer(const std::string& name);
    std::vector<LoadBalancerConfig> list_load_balancers() const;
    
    // VPN Management
    void create_vpn_tunnel(const std::string& name,
                          const std::string& remote_endpoint,
                          const std::string& pre_shared_key);
    void delete_vpn_tunnel(const std::string& name);
    void update_vpn_tunnel(const std::string& name,
                          const std::string& new_remote_endpoint,
                          const std::string& new_pre_shared_key);
    
    // Routing Management
    void add_route(const std::string& network,
                  const std::string& gateway,
                  const std::string& interface);
    void remove_route(const std::string& network);
    void update_route(const std::string& network,
                     const std::string& new_gateway,
                     const std::string& new_interface);
    
    // Monitoring
    void start_network_monitoring();
    void stop_network_monitoring();
    std::unordered_map<std::string, double> get_network_metrics() const;
    
    // Firewall Management
    void add_firewall_rule(const std::string& chain,
                          const std::string& protocol,
                          const std::string& source,
                          const std::string& destination,
                          const std::string& action);
    void remove_firewall_rule(const std::string& chain,
                             const std::string& protocol,
                             const std::string& source,
                             const std::string& destination);
    
    // QoS Management
    void configure_qos(const std::string& interface,
                      uint32_t bandwidth_limit,
                      uint32_t latency_limit);
    void remove_qos(const std::string& interface);

private:
    NetworkManager() = default;
    ~NetworkManager() = default;
    
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;

    struct Network {
        NetworkConfig config;
        std::vector<SecurityGroup> security_groups;
        std::chrono::system_clock::time_point created_at;
        bool is_active;
    };

    struct LoadBalancer {
        LoadBalancerConfig config;
        std::chrono::system_clock::time_point created_at;
        bool is_active;
        std::unordered_map<std::string, double> metrics;
    };

    std::unordered_map<std::string, Network> networks_;
    std::unordered_map<std::string, LoadBalancer> load_balancers_;
    mutable std::mutex networks_mutex_;
    mutable std::mutex load_balancers_mutex_;
    
    std::atomic<bool> monitoring_active_{false};
    std::thread monitoring_thread_;
    
    void monitoring_worker();
    void update_network_metrics();
    void check_network_health();
    void cleanup_inactive_resources();
    
    // Helper functions
    bool validate_network_config(const NetworkConfig& config) const;
    bool validate_security_group(const SecurityGroup& group) const;
    bool validate_load_balancer_config(const LoadBalancerConfig& config) const;
    void apply_network_changes(const Network& network);
    void apply_security_group_changes(const SecurityGroup& group);
    void apply_load_balancer_changes(const LoadBalancer& lb);
};

} // namespace network
} // namespace core 