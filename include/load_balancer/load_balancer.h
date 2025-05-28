#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <atomic>
#include <queue>
#include <functional>

namespace core {
namespace loadbalancer {

enum class Algorithm {
    ROUND_ROBIN,
    LEAST_CONNECTIONS,
    WEIGHTED_ROUND_ROBIN,
    LEAST_RESPONSE_TIME,
    IP_HASH,
    CONSISTENT_HASH
};

struct ServerStats {
    std::atomic<uint64_t> active_connections{0};
    std::atomic<uint64_t> total_requests{0};
    std::atomic<uint64_t> failed_requests{0};
    std::chrono::steady_clock::time_point last_health_check;
    double response_time_ms{0.0};
    bool is_healthy{true};
    uint32_t weight{1};
};

struct ServerConfig {
    std::string address;
    uint16_t port;
    uint32_t weight{1};
    uint32_t max_connections{1000};
    std::chrono::milliseconds health_check_interval{5000};
    std::chrono::milliseconds timeout{3000};
};

class LoadBalancer {
public:
    explicit LoadBalancer(Algorithm algorithm = Algorithm::ROUND_ROBIN);
    ~LoadBalancer();

    // Server Management
    void add_server(const ServerConfig& config);
    void remove_server(const std::string& address);
    void update_server_weight(const std::string& address, uint32_t weight);
    
    // Request Handling
    std::string get_next_server();
    void report_server_response(const std::string& address, bool success, 
                              std::chrono::milliseconds response_time);
    
    // Health Checks
    void start_health_checks();
    void stop_health_checks();
    
    // Statistics
    ServerStats get_server_stats(const std::string& address) const;
    std::vector<std::pair<std::string, ServerStats>> get_all_stats() const;

private:
    struct Server {
        ServerConfig config;
        ServerStats stats;
        std::chrono::steady_clock::time_point last_used;
    };

    Algorithm algorithm_;
    std::unordered_map<std::string, Server> servers_;
    mutable std::mutex servers_mutex_;
    std::atomic<size_t> current_server_index_{0};
    std::atomic<bool> health_checks_running_{false};
    
    // Algorithm-specific implementations
    std::string get_next_server_round_robin();
    std::string get_next_server_least_connections();
    std::string get_next_server_weighted_round_robin();
    std::string get_next_server_least_response_time();
    std::string get_next_server_ip_hash(const std::string& client_ip);
    std::string get_next_server_consistent_hash(const std::string& key);
    
    // Health check implementation
    void health_check_worker();
    bool check_server_health(const Server& server);
    
    // Helper functions
    void update_server_stats(const std::string& address, bool success, 
                           std::chrono::milliseconds response_time);
    void remove_unhealthy_server(const std::string& address);
};

} // namespace loadbalancer
} // namespace core 