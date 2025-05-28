#include "core/core_optimizer.h"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <chrono>
#include <thread>
#include <random>
#include <sstream>
#include <iomanip>

#ifdef __linux__
#include <sys/sysinfo.h>
#include <numa.h>
#elif defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/vm_statistics.h>
#endif

namespace core {

// Implementation of MultiLevelCache methods
template<typename T, size_t Levels>
bool MultiLevelCache<T, Levels>::try_get_level(size_t level, const void* key, T& value) {
    auto& cache_level = levels[level];
    size_t hash = std::hash<const void*>{}(key);
    size_t index = hash % cache_level.lines.size();
    
    std::shared_lock lock(cache_level.mutex);
    auto& line = cache_level.lines[index];
    
    if (line.valid) {
        value = line.data;
        line.last_access = std::chrono::steady_clock::now().time_since_epoch().count();
        return true;
    }
    
    return false;
}

template<typename T, size_t Levels>
bool MultiLevelCache<T, Levels>::try_put_level(size_t level, const void* key, const T& value) {
    auto& cache_level = levels[level];
    size_t hash = std::hash<const void*>{}(key);
    size_t index = hash % cache_level.lines.size();
    
    std::unique_lock lock(cache_level.mutex);
    auto& line = cache_level.lines[index];
    
    line.data = value;
    line.valid = true;
    line.last_access = std::chrono::steady_clock::now().time_since_epoch().count();
    
    return true;
}

// Implementation of AdvancedThreadPool methods
void AdvancedThreadPool::worker_thread(size_t thread_id) {
    auto& queue = queues[thread_id];
    
    while (!stop) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue.mutex);
            queue.cv.wait(lock, [&] {
                return stop || !queue.tasks.empty();
            });
            
            if (stop && queue.tasks.empty()) {
                return;
            }
            
            if (!queue.tasks.empty()) {
                task = std::move(queue.tasks.front());
                queue.tasks.pop();
            }
        }
        
        if (task) {
            task();
        }
    }
}

bool AdvancedThreadPool::try_steal_work(size_t thread_id) {
    size_t num_queues = queues.size();
    size_t start_idx = (thread_id + 1) % num_queues;
    
    for (size_t i = 0; i < num_queues - 1; ++i) {
        size_t victim_idx = (start_idx + i) % num_queues;
        auto& victim_queue = queues[victim_idx];
        
        std::unique_lock<std::mutex> lock(victim_queue.mutex, std::try_to_lock);
        if (!lock.owns_lock()) {
            continue;
        }
        
        if (!victim_queue.tasks.empty()) {
            auto task = std::move(victim_queue.tasks.front());
            victim_queue.tasks.pop();
            lock.unlock();
            
            task();
            return true;
        }
    }
    
    return false;
}

// Implementation of SIMDOperations methods
template<typename T>
void SIMDOperations::vector_add(T* dst, const T* src1, const T* src2, size_t count) {
#ifdef __AVX2__
    if constexpr (std::is_same_v<T, float>) {
        size_t i = 0;
        for (; i + 8 <= count; i += 8) {
            __m256 a = _mm256_loadu_ps(&src1[i]);
            __m256 b = _mm256_loadu_ps(&src2[i]);
            __m256 c = _mm256_add_ps(a, b);
            _mm256_storeu_ps(&dst[i], c);
        }
        for (; i < count; ++i) {
            dst[i] = src1[i] + src2[i];
        }
    } else if constexpr (std::is_same_v<T, double>) {
        size_t i = 0;
        for (; i + 4 <= count; i += 4) {
            __m256d a = _mm256_loadu_pd(&src1[i]);
            __m256d b = _mm256_loadu_pd(&src2[i]);
            __m256d c = _mm256_add_pd(a, b);
            _mm256_storeu_pd(&dst[i], c);
        }
        for (; i < count; ++i) {
            dst[i] = src1[i] + src2[i];
        }
    } else if constexpr (std::is_integral_v<T>) {
        size_t i = 0;
        for (; i + 8 <= count; i += 8) {
            __m256i a = _mm256_loadu_si256((__m256i*)&src1[i]);
            __m256i b = _mm256_loadu_si256((__m256i*)&src2[i]);
            __m256i c = _mm256_add_epi32(a, b);
            _mm256_storeu_si256((__m256i*)&dst[i], c);
        }
        for (; i < count; ++i) {
            dst[i] = src1[i] + src2[i];
        }
    }
#elif defined(__ARM_NEON)
    if constexpr (std::is_same_v<T, float>) {
        size_t i = 0;
        for (; i + 4 <= count; i += 4) {
            float32x4_t a = vld1q_f32(&src1[i]);
            float32x4_t b = vld1q_f32(&src2[i]);
            float32x4_t c = vaddq_f32(a, b);
            vst1q_f32(&dst[i], c);
        }
        for (; i < count; ++i) {
            dst[i] = src1[i] + src2[i];
        }
    } else if constexpr (std::is_same_v<T, double>) {
        size_t i = 0;
        for (; i + 2 <= count; i += 2) {
            float64x2_t a = vld1q_f64(&src1[i]);
            float64x2_t b = vld1q_f64(&src2[i]);
            float64x2_t c = vaddq_f64(a, b);
            vst1q_f64(&dst[i], c);
        }
        for (; i < count; ++i) {
            dst[i] = src1[i] + src2[i];
        }
    } else if constexpr (std::is_integral_v<T>) {
        size_t i = 0;
        for (; i + 4 <= count; i += 4) {
            int32x4_t a = vld1q_s32((int32_t*)&src1[i]);
            int32x4_t b = vld1q_s32((int32_t*)&src2[i]);
            int32x4_t c = vaddq_s32(a, b);
            vst1q_s32((int32_t*)&dst[i], c);
        }
        for (; i < count; ++i) {
            dst[i] = src1[i] + src2[i];
        }
    }
#else
    for (size_t i = 0; i < count; ++i) {
        dst[i] = src1[i] + src2[i];
    }
#endif
}

// Implementation of CoreOptimizer methods
void CoreOptimizer::detect_hardware() {
#ifdef __linux__
    // Detect CPU topology using sysfs
    for (size_t i = 0; i < MAX_CORES; ++i) {
        std::string path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/topology/core_id";
        std::ifstream file(path);
        if (file.is_open()) {
            file >> topology_.core_ids[i];
        }
    }
    
    // Detect NUMA nodes
    topology_.num_numa_nodes = numa_num_configured_nodes();
    for (size_t i = 0; i < MAX_CORES; ++i) {
        topology_.numa_nodes[i] = numa_node_of_cpu(i);
    }
    
    // Detect cache configuration
    std::ifstream cacheinfo("/sys/devices/system/cpu/cpu0/cache/index0/size");
    if (cacheinfo.is_open()) {
        std::string size;
        std::getline(cacheinfo, size);
        cache_config_.l1_size = std::stoull(size) * 1024;
    }
    
    cacheinfo.open("/sys/devices/system/cpu/cpu0/cache/index1/size");
    if (cacheinfo.is_open()) {
        std::string size;
        std::getline(cacheinfo, size);
        cache_config_.l2_size = std::stoull(size) * 1024;
    }
    
    cacheinfo.open("/sys/devices/system/cpu/cpu0/cache/index2/size");
    if (cacheinfo.is_open()) {
        std::string size;
        std::getline(cacheinfo, size);
        cache_config_.l3_size = std::stoull(size) * 1024;
    }
    
#elif defined(_WIN32)
    // Windows-specific hardware detection
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    topology_.num_cores = sysInfo.dwNumberOfProcessors;
    
    // Cache detection using CPUID
    int cpuInfo[4];
    __cpuid(cpuInfo, 0x80000006);
    cache_config_.l1_size = ((cpuInfo[2] >> 24) & 0xFF) * 1024;
    cache_config_.l2_size = ((cpuInfo[2] >> 16) & 0xFF) * 1024;
    cache_config_.l3_size = ((cpuInfo[3] >> 18) & 0x3FF) * 512 * 1024;
    
#elif defined(__APPLE__)
    // macOS-specific hardware detection
    size_t size = sizeof(topology_.num_cores);
    if (sysctlbyname("hw.ncpu", &topology_.num_cores, &size, nullptr, 0) != 0) {
        topology_.num_cores = 0;
    }
    
    // Cache detection
    size_t cacheSize;
    size = sizeof(cacheSize);
    if (sysctlbyname("hw.l1dcachesize", &cacheSize, &size, nullptr, 0) == 0) {
        cache_config_.l1_size = cacheSize;
    }
    if (sysctlbyname("hw.l2cachesize", &cacheSize, &size, nullptr, 0) == 0) {
        cache_config_.l2_size = cacheSize;
    }
    if (sysctlbyname("hw.l3cachesize", &cacheSize, &size, nullptr, 0) == 0) {
        cache_config_.l3_size = cacheSize;
    }
#endif

    // Set default values if detection failed
    if (cache_config_.l1_size == 0) cache_config_.l1_size = 32768;
    if (cache_config_.l2_size == 0) cache_config_.l2_size = 262144;
    if (cache_config_.l3_size == 0) cache_config_.l3_size = 8388608;
    
    cache_config_.line_size = CACHE_LINE_SIZE;
    cache_config_.associativity = 8;
}

void CoreOptimizer::setup_cache() {
    std::array<size_t, 3> sizes = {
        cache_config_.l1_size,
        cache_config_.l2_size,
        cache_config_.l3_size
    };
    std::array<size_t, 3> associativities = {8, 8, 16};
    cache_ = std::make_unique<MultiLevelCache<uint64_t, 3>>(sizes, associativities);
}

void CoreOptimizer::initialize_thread_pool() {
    thread_pool_ = std::make_unique<AdvancedThreadPool>(topology_.num_cores);
}

} // namespace core 