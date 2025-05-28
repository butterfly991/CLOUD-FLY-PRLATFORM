#include "load_balancer.h"
#include <algorithm>
#include <random>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

namespace core {
namespace loadbalancer {

LoadBalancer::LoadBalancer(Algorithm algorithm)
    : algorithm_(algorithm) {
}

LoadBalancer::~LoadBalancer() {
    stop_health_checks();
}

void LoadBalancer::add_server(const ServerConfig& config) {
    std::lock_guard<std::mutex> lock(servers_mutex_);
    Server server;
    server.config = config;
    server.stats.last_health_check = std::chrono::steady_clock::now();
    servers_[config.address] = std::move(server);
}

void LoadBalancer::remove_server(const std::string& address) {
    std::lock_guard<std::mutex> lock(servers_mutex_);
    servers_.erase(address);
}

void LoadBalancer::update_server_weight(const std::string& address, uint32_t weight) {
    std::lock_guard<std::mutex> lock(servers_mutex_);
    if (auto it = servers_.find(address); it != servers_.end()) {
        it->second.config.weight = weight;
        it->second.stats.weight = weight;
    }
}

std::string LoadBalancer::get_next_server() {
    std::lock_guard<std::mutex> lock(servers_mutex_);
    if (servers_.empty()) {
        return "";
    }

    switch (algorithm_) {
        case Algorithm::ROUND_ROBIN:
            return get_next_server_round_robin();
        case Algorithm::LEAST_CONNECTIONS:
            return get_next_server_least_connections();
        case Algorithm::WEIGHTED_ROUND_ROBIN:
            return get_next_server_weighted_round_robin();
        case Algorithm::LEAST_RESPONSE_TIME:
            return get_next_server_least_response_time();
        default:
            return get_next_server_round_robin();
    }
}

std::string LoadBalancer::get_next_server_round_robin() {
    if (servers_.empty()) return "";
    
    auto it = servers_.begin();
    std::advance(it, current_server_index_++ % servers_.size());
    it->second.stats.last_used = std::chrono::steady_clock::now();
    return it->first;
}

std::string LoadBalancer::get_next_server_least_connections() {
    if (servers_.empty()) return "";
    
    auto it = std::min_element(servers_.begin(), servers_.end(),
        [](const auto& a, const auto& b) {
            return a.second.stats.active_connections < b.second.stats.active_connections;
        });
    
    it->second.stats.last_used = std::chrono::steady_clock::now();
    return it->first;
}

std::string LoadBalancer::get_next_server_weighted_round_robin() {
    if (servers_.empty()) return "";
    
    uint32_t total_weight = 0;
    for (const auto& server : servers_) {
        total_weight += server.second.stats.weight;
    }
    
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(0, total_weight - 1);
    
    uint32_t random = dis(gen);
    uint32_t current_weight = 0;
    
    for (const auto& server : servers_) {
        current_weight += server.second.stats.weight;
        if (random < current_weight) {
            server.second.stats.last_used = std::chrono::steady_clock::now();
            return server.first;
        }
    }
    
    return servers_.begin()->first;
}

std::string LoadBalancer::get_next_server_least_response_time() {
    if (servers_.empty()) return "";
    
    auto it = std::min_element(servers_.begin(), servers_.end(),
        [](const auto& a, const auto& b) {
            return a.second.stats.response_time_ms < b.second.stats.response_time_ms;
        });
    
    it->second.stats.last_used = std::chrono::steady_clock::now();
    return it->first;
}

void LoadBalancer::report_server_response(const std::string& address, bool success,
                                        std::chrono::milliseconds response_time) {
    std::lock_guard<std::mutex> lock(servers_mutex_);
    if (auto it = servers_.find(address); it != servers_.end()) {
        update_server_stats(address, success, response_time);
    }
}

void LoadBalancer::update_server_stats(const std::string& address, bool success,
                                     std::chrono::milliseconds response_time) {
    auto& server = servers_[address];
    server.stats.total_requests++;
    if (!success) {
        server.stats.failed_requests++;
    }
    
    // Exponential moving average for response time
    const double alpha = 0.1;
    server.stats.response_time_ms = alpha * response_time.count() +
                                  (1 - alpha) * server.stats.response_time_ms;
}

void LoadBalancer::start_health_checks() {
    if (health_checks_running_.exchange(true)) {
        return;
    }
    
    std::thread([this]() {
        while (health_checks_running_) {
            std::lock_guard<std::mutex> lock(servers_mutex_);
            for (auto& [address, server] : servers_) {
                if (check_server_health(server)) {
                    server.stats.is_healthy = true;
                } else {
                    server.stats.is_healthy = false;
                    remove_unhealthy_server(address);
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }).detach();
}

void LoadBalancer::stop_health_checks() {
    health_checks_running_ = false;
}

bool LoadBalancer::check_server_health(const Server& server) {
    // Implement actual health check logic here
    // This is a placeholder implementation
    return true;
}

void LoadBalancer::remove_unhealthy_server(const std::string& address) {
    servers_.erase(address);
}

ServerStats LoadBalancer::get_server_stats(const std::string& address) const {
    std::lock_guard<std::mutex> lock(servers_mutex_);
    if (auto it = servers_.find(address); it != servers_.end()) {
        return it->second.stats;
    }
    return ServerStats{};
}

std::vector<std::pair<std::string, ServerStats>> LoadBalancer::get_all_stats() const {
    std::lock_guard<std::mutex> lock(servers_mutex_);
    std::vector<std::pair<std::string, ServerStats>> result;
    result.reserve(servers_.size());
    
    for (const auto& [address, server] : servers_) {
        result.emplace_back(address, server.stats);
    }
    
    return result;
}

} // namespace loadbalancer
} // namespace core 