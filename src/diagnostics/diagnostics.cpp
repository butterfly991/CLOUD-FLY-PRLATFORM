#include "diagnostics.h"
#include "architecture.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstring>
#include <algorithm>

namespace core {
namespace diagnostics {

// DiagnosticProbe Implementation
class DiagnosticProbe::Impl {
public:
    std::vector<MetricCallback> metric_callbacks;
    std::vector<ITelemetrySink::Ptr> sinks;
    std::atomic<bool> is_running{false};
    std::thread flush_thread;
    std::mutex callback_mutex;
    std::mutex sink_mutex;
    
    // Architecture-specific members
    #if defined(__ARM_NEON)
        // ARM-specific performance counters
        uint64_t arm_cycle_count{0};
        uint64_t arm_instruction_count{0};
    #endif
};

DiagnosticProbe::DiagnosticProbe() : impl_(std::make_unique<Impl>()) {}

DiagnosticProbe::~DiagnosticProbe() {
    stop();
}

void DiagnosticProbe::start() {
    if (impl_->is_running.exchange(true)) {
        return;
    }
    
    // Start flush thread
    impl_->flush_thread = std::thread([this]() {
        while (impl_->is_running) {
            flush_metrics();
            std::this_thread::sleep_for(DEFAULT_FLUSH_INTERVAL);
        }
    });
}

void DiagnosticProbe::stop() {
    if (!impl_->is_running.exchange(false)) {
        return;
    }
    
    if (impl_->flush_thread.joinable()) {
        impl_->flush_thread.join();
    }
    
    flush_metrics();
}

void DiagnosticProbe::register_metric_callback(MetricCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->callback_mutex);
    impl_->metric_callbacks.push_back(std::move(callback));
}

void DiagnosticProbe::register_sink(ITelemetrySink::Ptr sink) {
    std::lock_guard<std::mutex> lock(impl_->sink_mutex);
    impl_->sinks.push_back(std::move(sink));
}

void DiagnosticProbe::record_metric(const CompactMetric& metric) {
    // Call registered callbacks
    {
        std::lock_guard<std::mutex> lock(impl_->callback_mutex);
        for (const auto& callback : impl_->metric_callbacks) {
            callback(metric);
        }
    }
    
    // Forward to sinks
    {
        std::lock_guard<std::mutex> lock(impl_->sink_mutex);
        for (const auto& sink : impl_->sinks) {
            sink->push_metric(metric);
        }
    }
}

void DiagnosticProbe::flush_metrics() {
    std::lock_guard<std::mutex> lock(impl_->sink_mutex);
    for (const auto& sink : impl_->sinks) {
        sink->flush();
    }
}

// FileTelemetrySink Implementation
class FileTelemetrySink : public ITelemetrySink {
public:
    explicit FileTelemetrySink(const std::string& path)
        : file_(path, std::ios::app) {
        if (!file_) {
            throw std::runtime_error("Failed to open telemetry file: " + path);
        }
    }
    
    void start() override {
        // No-op
    }
    
    void stop() noexcept override {
        std::lock_guard<std::mutex> lock(mutex_);
        file_.close();
    }
    
    void flush() override {
        std::lock_guard<std::mutex> lock(mutex_);
        file_.flush();
    }
    
    void log(LogLevel level, std::string_view message) noexcept override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!file_) return;
        
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        
        file_ << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
              << " [" << level_to_string(level) << "] "
              << message << std::endl;
    }
    
    void log_failure(FailureType type, std::string_view context) noexcept override {
        log(LogLevel::Error, "Failure: " + std::string(context));
    }
    
    void push_metric(const CompactMetric& metric) noexcept override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!file_) return;
        
        file_ << "METRIC " << metric.name << " "
              << std::visit([](const auto& v) { return std::to_string(v); }, metric.value)
              << std::endl;
    }
    
    void push_metrics_batch(std::span<const CompactMetric> metrics) noexcept override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!file_) return;
        
        for (const auto& metric : metrics) {
            push_metric(metric);
        }
    }
    
    void set_sampling_rate(uint32_t samples_per_sec) noexcept override {
        // No-op
    }
    
    void set_sampling_mode(SamplingMode mode) noexcept override {
        // No-op
    }
    
    void set_error_handler(Callback&& handler) noexcept override {
        error_handler_ = std::move(handler);
    }
    
private:
    std::ofstream file_;
    std::mutex mutex_;
    Callback error_handler_;
    
    static const char* level_to_string(LogLevel level) {
        switch (level) {
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info: return "INFO";
            case LogLevel::Warning: return "WARNING";
            case LogLevel::Error: return "ERROR";
            case LogLevel::Critical: return "CRITICAL";
            case LogLevel::Fatal: return "FATAL";
            default: return "UNKNOWN";
        }
    }
};

// Factory function for creating telemetry sinks
ITelemetrySink::Ptr create_file_sink(const std::string& path) {
    return std::make_unique<FileTelemetrySink>(path);
}

} // namespace diagnostics
} // namespace core 