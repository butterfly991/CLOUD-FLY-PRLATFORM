#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <memory>
#include <functional>
#include <mutex>
#include <queue>
#include <atomic>
#include <thread>

namespace core {
namespace monitoring {

enum class MetricType {
    COUNTER,
    GAUGE,
    HISTOGRAM,
    SUMMARY
};

enum class AlertSeverity {
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

struct MetricValue {
    double value;
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, std::string> labels;
};

struct Alert {
    std::string name;
    std::string description;
    AlertSeverity severity;
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, std::string> labels;
    bool is_active;
};

struct MetricDefinition {
    std::string name;
    MetricType type;
    std::string description;
    std::vector<std::string> label_names;
    std::function<bool(double)> alert_condition;
    AlertSeverity alert_severity;
};

class MonitoringSystem {
public:
    static MonitoringSystem& get_instance() {
        static MonitoringSystem instance;
        return instance;
    }

    // Metric Management
    void register_metric(const MetricDefinition& definition);
    void record_metric(const std::string& name, double value,
                      const std::unordered_map<std::string, std::string>& labels = {});
    std::vector<MetricValue> get_metric_values(const std::string& name,
                                             std::chrono::seconds time_window) const;
    
    // Alert Management
    void register_alert_handler(std::function<void(const Alert&)> handler);
    void clear_alert(const std::string& name);
    std::vector<Alert> get_active_alerts() const;
    
    // System Health
    void start_monitoring();
    void stop_monitoring();
    bool is_healthy() const;
    
    // Resource Monitoring
    void monitor_cpu_usage();
    void monitor_memory_usage();
    void monitor_disk_usage();
    void monitor_network_traffic();
    
    // Performance Metrics
    void record_response_time(const std::string& endpoint, std::chrono::milliseconds duration);
    void record_error_rate(const std::string& service, double rate);
    void record_throughput(const std::string& service, uint64_t requests_per_second);

private:
    MonitoringSystem() = default;
    ~MonitoringSystem() = default;
    
    MonitoringSystem(const MonitoringSystem&) = delete;
    MonitoringSystem& operator=(const MonitoringSystem&) = delete;

    struct Metric {
        MetricDefinition definition;
        std::vector<MetricValue> values;
        mutable std::mutex mutex;
    };

    std::unordered_map<std::string, Metric> metrics_;
    std::vector<std::function<void(const Alert&)>> alert_handlers_;
    std::vector<Alert> active_alerts_;
    mutable std::mutex metrics_mutex_;
    mutable std::mutex alerts_mutex_;
    
    std::atomic<bool> monitoring_active_{false};
    std::thread monitoring_thread_;
    
    void monitoring_worker();
    void check_alert_conditions();
    void cleanup_old_metrics();
    void process_alert(const Alert& alert);
    
    // Helper functions
    double calculate_percentile(const std::vector<double>& values, double percentile) const;
    void update_metric_statistics(Metric& metric, double value);
    bool evaluate_alert_condition(const MetricDefinition& definition, double value) const;
};

} // namespace monitoring
} // namespace core 