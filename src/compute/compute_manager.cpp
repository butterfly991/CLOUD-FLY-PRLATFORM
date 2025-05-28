#include "compute/compute_manager.h"
#include <iostream>
#include <algorithm>
#include <thread>
#include <queue>
#include <condition_variable>
#include <chrono>

#ifdef __x86_64__
#include <immintrin.h>
#endif

#ifdef __aarch64__
#include <arm_neon.h>
#endif

namespace compute {

ComputeManager& ComputeManager::getInstance() {
    static ComputeManager instance;
    return instance;
}

ComputeManager::ComputeManager() {
    initialize();
}

ComputeManager::~ComputeManager() {
    shutdown();
}

void ComputeManager::initialize() {
    if (initialized_) return;

    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) return;

    initializeThreadPool();
    stats_.startTime = std::chrono::steady_clock::now();
    initialized_ = true;
}

void ComputeManager::shutdown() {
    if (!initialized_) return;

    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_) return;

    cleanupThreadPool();
    initialized_ = false;
}

void ComputeManager::setThreadCount(size_t count) {
    if (count == 0 || count > MAX_THREADS) return;
    threadCount_ = count;
    
    if (initialized_) {
        cleanupThreadPool();
        initializeThreadPool();
    }
}

void ComputeManager::setBatchSize(size_t size) {
    if (size == 0) return;
    batchSize_ = size;
}

void ComputeManager::initializeThreadPool() {
    threadPool_ = std::make_unique<ThreadPool>();
    threadPool_->stop = false;
    
    for (size_t i = 0; i < threadCount_; ++i) {
        threadPool_->threads.emplace_back(&ComputeManager::workerThread, this);
    }
}

void ComputeManager::cleanupThreadPool() {
    if (!threadPool_) return;

    {
        std::lock_guard<std::mutex> lock(threadPool_->mutex);
        threadPool_->stop = true;
    }
    threadPool_->condition.notify_all();

    for (auto& thread : threadPool_->threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    threadPool_->threads.clear();
    threadPool_.reset();
}

void ComputeManager::workerThread() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(threadPool_->mutex);
            threadPool_->condition.wait(lock, [this] {
                return threadPool_->stop || !threadPool_->tasks.empty();
            });

            if (threadPool_->stop && threadPool_->tasks.empty()) {
                return;
            }

            task = std::move(threadPool_->tasks.front());
            threadPool_->tasks.pop();
        }
        task();
    }
}

void ComputeManager::updateStats(OperationType type, size_t count, bool simd) {
    stats_.totalOperations += count;
    if (simd) {
        stats_.simdOperations += count;
    } else {
        stats_.scalarOperations += count;
    }
}

ComputeStats ComputeManager::getStats() const {
    return stats_;
}

void ComputeManager::resetStats() {
    stats_ = ComputeStats();
    stats_.startTime = std::chrono::steady_clock::now();
}

size_t ComputeManager::getThreadCount() const {
    return threadCount_;
}

size_t ComputeManager::getBatchSize() const {
    return batchSize_;
}

bool ComputeManager::isSIMDAvailable() const {
#ifdef __x86_64__
    return true;
#elif defined(__aarch64__)
    return true;
#else
    return false;
#endif
}

bool ComputeManager::isAVXAvailable() const {
#ifdef __x86_64__
    return __builtin_cpu_supports("avx");
#else
    return false;
#endif
}

bool ComputeManager::isNEONAvailable() const {
#ifdef __aarch64__
    return true;
#else
    return false;
#endif
}

// SIMD оптимизации для x86_64
#ifdef __x86_64__

template<typename T>
void ComputeManager::simdAdd(T* dst, const T* src1, const T* src2, size_t count) {
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
        for (; i + 16 <= count; i += 16) {
            __m256i a = _mm256_loadu_si256((__m256i*)&src1[i]);
            __m256i b = _mm256_loadu_si256((__m256i*)&src2[i]);
            __m256i c = _mm256_add_epi32(a, b);
            _mm256_storeu_si256((__m256i*)&dst[i], c);
        }
        for (; i < count; ++i) {
            dst[i] = src1[i] + src2[i];
        }
    }
}

template<typename T>
void ComputeManager::simdMultiply(T* dst, const T* src1, const T* src2, size_t count) {
    if constexpr (std::is_same_v<T, float>) {
        size_t i = 0;
        for (; i + 8 <= count; i += 8) {
            __m256 a = _mm256_loadu_ps(&src1[i]);
            __m256 b = _mm256_loadu_ps(&src2[i]);
            __m256 c = _mm256_mul_ps(a, b);
            _mm256_storeu_ps(&dst[i], c);
        }
        for (; i < count; ++i) {
            dst[i] = src1[i] * src2[i];
        }
    } else if constexpr (std::is_same_v<T, double>) {
        size_t i = 0;
        for (; i + 4 <= count; i += 4) {
            __m256d a = _mm256_loadu_pd(&src1[i]);
            __m256d b = _mm256_loadu_pd(&src2[i]);
            __m256d c = _mm256_mul_pd(a, b);
            _mm256_storeu_pd(&dst[i], c);
        }
        for (; i < count; ++i) {
            dst[i] = src1[i] * src2[i];
        }
    } else if constexpr (std::is_integral_v<T>) {
        size_t i = 0;
        for (; i + 8 <= count; i += 8) {
            __m256i a = _mm256_loadu_si256((__m256i*)&src1[i]);
            __m256i b = _mm256_loadu_si256((__m256i*)&src2[i]);
            __m256i c = _mm256_mullo_epi32(a, b);
            _mm256_storeu_si256((__m256i*)&dst[i], c);
        }
        for (; i < count; ++i) {
            dst[i] = src1[i] * src2[i];
        }
    }
}

template<typename T>
T ComputeManager::simdSum(const T* data, size_t count) {
    if constexpr (std::is_same_v<T, float>) {
        __m256 sum = _mm256_setzero_ps();
        size_t i = 0;
        for (; i + 8 <= count; i += 8) {
            __m256 a = _mm256_loadu_ps(&data[i]);
            sum = _mm256_add_ps(sum, a);
        }
        float result = 0.0f;
        for (; i < count; ++i) {
            result += data[i];
        }
        float temp[8];
        _mm256_storeu_ps(temp, sum);
        for (int j = 0; j < 8; ++j) {
            result += temp[j];
        }
        return result;
    } else if constexpr (std::is_same_v<T, double>) {
        __m256d sum = _mm256_setzero_pd();
        size_t i = 0;
        for (; i + 4 <= count; i += 4) {
            __m256d a = _mm256_loadu_pd(&data[i]);
            sum = _mm256_add_pd(sum, a);
        }
        double result = 0.0;
        for (; i < count; ++i) {
            result += data[i];
        }
        double temp[4];
        _mm256_storeu_pd(temp, sum);
        for (int j = 0; j < 4; ++j) {
            result += temp[j];
        }
        return result;
    } else if constexpr (std::is_integral_v<T>) {
        __m256i sum = _mm256_setzero_si256();
        size_t i = 0;
        for (; i + 8 <= count; i += 8) {
            __m256i a = _mm256_loadu_si256((__m256i*)&data[i]);
            sum = _mm256_add_epi32(sum, a);
        }
        T result = 0;
        for (; i < count; ++i) {
            result += data[i];
        }
        int32_t temp[8];
        _mm256_storeu_si256((__m256i*)temp, sum);
        for (int j = 0; j < 8; ++j) {
            result += temp[j];
        }
        return result;
    }
    return T();
}

template<typename T>
T ComputeManager::simdDotProduct(const T* vec1, const T* vec2, size_t count) {
    if constexpr (std::is_same_v<T, float>) {
        __m256 sum = _mm256_setzero_ps();
        size_t i = 0;
        for (; i + 8 <= count; i += 8) {
            __m256 a = _mm256_loadu_ps(&vec1[i]);
            __m256 b = _mm256_loadu_ps(&vec2[i]);
            __m256 c = _mm256_mul_ps(a, b);
            sum = _mm256_add_ps(sum, c);
        }
        float result = 0.0f;
        for (; i < count; ++i) {
            result += vec1[i] * vec2[i];
        }
        float temp[8];
        _mm256_storeu_ps(temp, sum);
        for (int j = 0; j < 8; ++j) {
            result += temp[j];
        }
        return result;
    } else if constexpr (std::is_same_v<T, double>) {
        __m256d sum = _mm256_setzero_pd();
        size_t i = 0;
        for (; i + 4 <= count; i += 4) {
            __m256d a = _mm256_loadu_pd(&vec1[i]);
            __m256d b = _mm256_loadu_pd(&vec2[i]);
            __m256d c = _mm256_mul_pd(a, b);
            sum = _mm256_add_pd(sum, c);
        }
        double result = 0.0;
        for (; i < count; ++i) {
            result += vec1[i] * vec2[i];
        }
        double temp[4];
        _mm256_storeu_pd(temp, sum);
        for (int j = 0; j < 4; ++j) {
            result += temp[j];
        }
        return result;
    } else if constexpr (std::is_integral_v<T>) {
        __m256i sum = _mm256_setzero_si256();
        size_t i = 0;
        for (; i + 8 <= count; i += 8) {
            __m256i a = _mm256_loadu_si256((__m256i*)&vec1[i]);
            __m256i b = _mm256_loadu_si256((__m256i*)&vec2[i]);
            __m256i c = _mm256_mullo_epi32(a, b);
            sum = _mm256_add_epi32(sum, c);
        }
        T result = 0;
        for (; i < count; ++i) {
            result += vec1[i] * vec2[i];
        }
        int32_t temp[8];
        _mm256_storeu_si256((__m256i*)temp, sum);
        for (int j = 0; j < 8; ++j) {
            result += temp[j];
        }
        return result;
    }
    return T();
}

// SIMD оптимизации для ARM64
#elif defined(__aarch64__)

template<typename T>
void ComputeManager::simdAdd(T* dst, const T* src1, const T* src2, size_t count) {
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
}

template<typename T>
void ComputeManager::simdMultiply(T* dst, const T* src1, const T* src2, size_t count) {
    if constexpr (std::is_same_v<T, float>) {
        size_t i = 0;
        for (; i + 4 <= count; i += 4) {
            float32x4_t a = vld1q_f32(&src1[i]);
            float32x4_t b = vld1q_f32(&src2[i]);
            float32x4_t c = vmulq_f32(a, b);
            vst1q_f32(&dst[i], c);
        }
        for (; i < count; ++i) {
            dst[i] = src1[i] * src2[i];
        }
    } else if constexpr (std::is_same_v<T, double>) {
        size_t i = 0;
        for (; i + 2 <= count; i += 2) {
            float64x2_t a = vld1q_f64(&src1[i]);
            float64x2_t b = vld1q_f64(&src2[i]);
            float64x2_t c = vmulq_f64(a, b);
            vst1q_f64(&dst[i], c);
        }
        for (; i < count; ++i) {
            dst[i] = src1[i] * src2[i];
        }
    } else if constexpr (std::is_integral_v<T>) {
        size_t i = 0;
        for (; i + 4 <= count; i += 4) {
            int32x4_t a = vld1q_s32((int32_t*)&src1[i]);
            int32x4_t b = vld1q_s32((int32_t*)&src2[i]);
            int32x4_t c = vmulq_s32(a, b);
            vst1q_s32((int32_t*)&dst[i], c);
        }
        for (; i < count; ++i) {
            dst[i] = src1[i] * src2[i];
        }
    }
}

template<typename T>
T ComputeManager::simdSum(const T* data, size_t count) {
    if constexpr (std::is_same_v<T, float>) {
        float32x4_t sum = vdupq_n_f32(0.0f);
        size_t i = 0;
        for (; i + 4 <= count; i += 4) {
            float32x4_t a = vld1q_f32(&data[i]);
            sum = vaddq_f32(sum, a);
        }
        float result = 0.0f;
        for (; i < count; ++i) {
            result += data[i];
        }
        float temp[4];
        vst1q_f32(temp, sum);
        for (int j = 0; j < 4; ++j) {
            result += temp[j];
        }
        return result;
    } else if constexpr (std::is_same_v<T, double>) {
        float64x2_t sum = vdupq_n_f64(0.0);
        size_t i = 0;
        for (; i + 2 <= count; i += 2) {
            float64x2_t a = vld1q_f64(&data[i]);
            sum = vaddq_f64(sum, a);
        }
        double result = 0.0;
        for (; i < count; ++i) {
            result += data[i];
        }
        double temp[2];
        vst1q_f64(temp, sum);
        for (int j = 0; j < 2; ++j) {
            result += temp[j];
        }
        return result;
    } else if constexpr (std::is_integral_v<T>) {
        int32x4_t sum = vdupq_n_s32(0);
        size_t i = 0;
        for (; i + 4 <= count; i += 4) {
            int32x4_t a = vld1q_s32((int32_t*)&data[i]);
            sum = vaddq_s32(sum, a);
        }
        T result = 0;
        for (; i < count; ++i) {
            result += data[i];
        }
        int32_t temp[4];
        vst1q_s32(temp, sum);
        for (int j = 0; j < 4; ++j) {
            result += temp[j];
        }
        return result;
    }
    return T();
}

template<typename T>
T ComputeManager::simdDotProduct(const T* vec1, const T* vec2, size_t count) {
    if constexpr (std::is_same_v<T, float>) {
        float32x4_t sum = vdupq_n_f32(0.0f);
        size_t i = 0;
        for (; i + 4 <= count; i += 4) {
            float32x4_t a = vld1q_f32(&vec1[i]);
            float32x4_t b = vld1q_f32(&vec2[i]);
            float32x4_t c = vmulq_f32(a, b);
            sum = vaddq_f32(sum, c);
        }
        float result = 0.0f;
        for (; i < count; ++i) {
            result += vec1[i] * vec2[i];
        }
        float temp[4];
        vst1q_f32(temp, sum);
        for (int j = 0; j < 4; ++j) {
            result += temp[j];
        }
        return result;
    } else if constexpr (std::is_same_v<T, double>) {
        float64x2_t sum = vdupq_n_f64(0.0);
        size_t i = 0;
        for (; i + 2 <= count; i += 2) {
            float64x2_t a = vld1q_f64(&vec1[i]);
            float64x2_t b = vld1q_f64(&vec2[i]);
            float64x2_t c = vmulq_f64(a, b);
            sum = vaddq_f64(sum, c);
        }
        double result = 0.0;
        for (; i < count; ++i) {
            result += vec1[i] * vec2[i];
        }
        double temp[2];
        vst1q_f64(temp, sum);
        for (int j = 0; j < 2; ++j) {
            result += temp[j];
        }
        return result;
    } else if constexpr (std::is_integral_v<T>) {
        int32x4_t sum = vdupq_n_s32(0);
        size_t i = 0;
        for (; i + 4 <= count; i += 4) {
            int32x4_t a = vld1q_s32((int32_t*)&vec1[i]);
            int32x4_t b = vld1q_s32((int32_t*)&vec2[i]);
            int32x4_t c = vmulq_s32(a, b);
            sum = vaddq_s32(sum, c);
        }
        T result = 0;
        for (; i < count; ++i) {
            result += vec1[i] * vec2[i];
        }
        int32_t temp[4];
        vst1q_s32(temp, sum);
        for (int j = 0; j < 4; ++j) {
            result += temp[j];
        }
        return result;
    }
    return T();
}

#else

// Заглушки для платформ без SIMD
template<typename T>
void ComputeManager::simdAdd(T* dst, const T* src1, const T* src2, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        dst[i] = src1[i] + src2[i];
    }
}

template<typename T>
void ComputeManager::simdMultiply(T* dst, const T* src1, const T* src2, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        dst[i] = src1[i] * src2[i];
    }
}

template<typename T>
T ComputeManager::simdSum(const T* data, size_t count) {
    T result = T();
    for (size_t i = 0; i < count; ++i) {
        result += data[i];
    }
    return result;
}

template<typename T>
T ComputeManager::simdDotProduct(const T* vec1, const T* vec2, size_t count) {
    T result = T();
    for (size_t i = 0; i < count; ++i) {
        result += vec1[i] * vec2[i];
    }
    return result;
}

#endif

} // namespace compute 