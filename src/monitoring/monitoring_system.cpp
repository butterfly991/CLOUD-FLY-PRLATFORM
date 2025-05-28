#include "monitoring_system.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

namespace core {
namespace monitoring {

void MonitoringSystem::register_metric(const MetricDefinition& definition) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    metrics_[definition.name] = Metric{definition, {}, {}};
}

void MonitoringSystem::record_metric(const std::string& name, double value,
                                   const std::unordered_map<std::string, std::string>& labels) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    if (auto it = metrics_.find(name); it != metrics_.end()) {
        MetricValue metric_value{
            value,
            std::chrono::system_clock::now(),
            labels
        };
        
        std::lock_guard<std::mutex> metric_lock(it->second.mutex);
        it->second.values.push_back(std::move(metric_value));
        update_metric_statistics(it->second, value);
        
        if (evaluate_alert_condition(it->second.definition, value)) {
            Alert alert{
                name,
                "Metric value exceeded threshold",
                it->second.definition.alert_severity,
                std::chrono::system_clock::now(),
                labels,
                true
            };
            process_alert(alert);
        }
    }
}

std::vector<MetricValue> MonitoringSystem::get_metric_values(
    const std::string& name,
    std::chrono::seconds time_window) const {
    
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    if (auto it = metrics_.find(name); it != metrics_.end()) {
        std::lock_guard<std::mutex> metric_lock(it->second.mutex);
        auto now = std::chrono::system_clock::now();
        std::vector<MetricValue> result;
        
        std::copy_if(it->second.values.begin(), it->second.values.end(),
                    std::back_inserter(result),
                    [&](const MetricValue& value) {
                        return (now - value.timestamp) <= time_window;
                    });
        
        return result;
    }
    return {};
}

void MonitoringSystem::register_alert_handler(std::function<void(const Alert&)> handler) {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    alert_handlers_.push_back(std::move(handler));
}

void MonitoringSystem::clear_alert(const std::string& name) {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    active_alerts_.erase(
        std::remove_if(active_alerts_.begin(), active_alerts_.end(),
                      [&](const Alert& alert) { return alert.name == name; }),
        active_alerts_.end()
    );
}

std::vector<Alert> MonitoringSystem::get_active_alerts() const {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    return active_alerts_;
}

void MonitoringSystem::start_monitoring() {
    if (monitoring_active_.exchange(true)) {
        return;
    }
    
    monitoring_thread_ = std::thread([this]() {
        monitoring_worker();
    });
}

void MonitoringSystem::stop_monitoring() {
    monitoring_active_ = false;
    if (monitoring_thread_.joinable()) {
        monitoring_thread_.join();
    }
}

bool MonitoringSystem::is_healthy() const {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    return std::none_of(active_alerts_.begin(), active_alerts_.end(),
                       [](const Alert& alert) {
                           return alert.severity == AlertSeverity::CRITICAL;
                       });
}

void MonitoringSystem::monitor_cpu_usage() {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        double cpu_time = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1000000.0;
        record_metric("cpu_usage", cpu_time, {{"type", "user"}});
    }
}

void MonitoringSystem::monitor_memory_usage() {
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        double memory_usage = (si.totalram - si.freeram) * 100.0 / si.totalram;
        record_metric("memory_usage", memory_usage);
    }
}

void MonitoringSystem::monitor_disk_usage() {
    struct statvfs sf;
    if (statvfs(".", &sf) == 0) {
        double disk_usage = (sf.f_blocks - sf.f_bfree) * 100.0 / sf.f_blocks;
        record_metric("disk_usage", disk_usage);
    }
}

void MonitoringSystem::monitor_network_traffic() {
    // Implementation would depend on the specific network interface
    // This is a placeholder for the actual implementation
    record_metric("network_traffic", 0.0, {{"interface", "eth0"}});
}

void MonitoringSystem::record_response_time(const std::string& endpoint,
                                          std::chrono::milliseconds duration) {
    record_metric("response_time", duration.count(),
                 {{"endpoint", endpoint}});
}

void MonitoringSystem::record_error_rate(const std::string& service, double rate) {
    record_metric("error_rate", rate,
                 {{"service", service}});
}

void MonitoringSystem::record_throughput(const std::string& service,
                                       uint64_t requests_per_second) {
    record_metric("throughput", static_cast<double>(requests_per_second),
                 {{"service", service}});
}

void MonitoringSystem::monitoring_worker() {
    while (monitoring_active_) {
        monitor_cpu_usage();
        monitor_memory_usage();
        monitor_disk_usage();
        monitor_network_traffic();
        check_alert_conditions();
        cleanup_old_metrics();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void MonitoringSystem::check_alert_conditions() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    for (const auto& [name, metric] : metrics_) {
        if (metric.definition.alert_condition) {
            std::lock_guard<std::mutex> metric_lock(metric.mutex);
            if (!metric.values.empty()) {
                double latest_value = metric.values.back().value;
                if (evaluate_alert_condition(metric.definition, latest_value)) {
                    Alert alert{
                        name,
                        "Metric value exceeded threshold",
                        metric.definition.alert_severity,
                        std::chrono::system_clock::now(),
                        metric.values.back().labels,
                        true
                    };
                    process_alert(alert);
                }
            }
        }
    }
}

void MonitoringSystem::cleanup_old_metrics() {
    const auto retention_period = std::chrono::hours(24);
    auto now = std::chrono::system_clock::now();
    
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    for (auto& [name, metric] : metrics_) {
        std::lock_guard<std::mutex> metric_lock(metric.mutex);
        metric.values.erase(
            std::remove_if(metric.values.begin(), metric.values.end(),
                          [&](const MetricValue& value) {
                              return (now - value.timestamp) > retention_period;
                          }),
            metric.values.end()
        );
    }
}

void MonitoringSystem::process_alert(const Alert& alert) {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    active_alerts_.push_back(alert);
    
    for (const auto& handler : alert_handlers_) {
        handler(alert);
    }
}

double MonitoringSystem::calculate_percentile(const std::vector<double>& values,
                                           double percentile) const {
    if (values.empty()) return 0.0;
    
    std::vector<double> sorted_values = values;
    std::sort(sorted_values.begin(), sorted_values.end());
    
    size_t index = static_cast<size_t>(std::ceil(percentile / 100.0 * sorted_values.size())) - 1;
    return sorted_values[index];
}

void MonitoringSystem::update_metric_statistics(Metric& metric, double value) {
    // Implementation would depend on the metric type
    // This is a placeholder for the actual implementation
}

bool MonitoringSystem::evaluate_alert_condition(const MetricDefinition& definition,
                                              double value) const {
    return definition.alert_condition && definition.alert_condition(value);
}

} // namespace monitoring
} // namespace core 