#pragma once

#include <memory>
#include <vector>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <string>
#include <functional>
#include <cstdint>
#include <array>
#include <bitset>

namespace memory {

// Константы для настройки памяти
constexpr size_t DEFAULT_PAGE_SIZE = 4096;
constexpr size_t DEFAULT_ALIGNMENT = 64;
constexpr size_t MAX_POOL_SIZE = 1024 * 1024 * 1024; // 1GB
constexpr size_t MIN_BLOCK_SIZE = 16;
constexpr size_t MAX_BLOCK_SIZE = 1024 * 1024; // 1MB

// Структура для статистики памяти
struct MemoryStats {
    std::atomic<uint64_t> totalAllocated{0};
    std::atomic<uint64_t> totalFreed{0};
    std::atomic<uint64_t> peakUsage{0};
    std::atomic<uint64_t> currentUsage{0};
    std::atomic<uint64_t> allocationCount{0};
    std::atomic<uint64_t> freeCount{0};
    std::atomic<uint64_t> pageFaults{0};
    std::atomic<uint64_t> cacheHits{0};
    std::atomic<uint64_t> cacheMisses{0};
};

// Класс для управления памятью
class MemoryManager {
public:
    static MemoryManager& getInstance();
    
    // Инициализация и управление
    void initialize();
    void shutdown();
    void setMemoryLimit(size_t limit);
    
    // Аллокация и освобождение памяти
    void* allocate(size_t size, size_t alignment = DEFAULT_ALIGNMENT);
    void deallocate(void* ptr);
    void* reallocate(void* ptr, size_t newSize);
    
    // Операции с памятью
    void prefetch(const void* ptr, size_t size);
    void flush(const void* ptr, size_t size);
    void zero(void* ptr, size_t size);
    void copy(void* dst, const void* src, size_t size);
    void move(void* dst, const void* src, size_t size);
    
    // Оптимизированные операции
    template<typename T>
    void fill(T* ptr, const T& value, size_t count);
    
    template<typename T>
    void copy(T* dst, const T* src, size_t count);
    
    // Получение статистики
    MemoryStats getStats() const;
    void resetStats();
    
    // Утилиты
    size_t getPageSize() const;
    size_t getTotalMemory() const;
    size_t getAvailableMemory() const;
    size_t getMemoryLimit() const;
    bool isMemoryAvailable(size_t size) const;

private:
    MemoryManager();
    ~MemoryManager();
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    // Внутренние структуры
    struct MemoryBlock {
        void* ptr;
        size_t size;
        size_t alignment;
        bool isUsed;
        std::chrono::steady_clock::time_point lastAccess;
    };

    struct MemoryPool {
        std::vector<MemoryBlock> blocks;
        std::mutex mutex;
        size_t totalSize;
        size_t usedSize;
    };

    // Внутренние методы
    void initializePools();
    void cleanupPools();
    MemoryBlock* findFreeBlock(size_t size, size_t alignment);
    void defragment();
    void updateStats(size_t allocated, size_t freed);
    
    // SIMD оптимизации
    void* allocateAligned(size_t size, size_t alignment);
    void deallocateAligned(void* ptr);
    
    // Кэширование
    void* getFromCache(size_t size);
    void putToCache(void* ptr, size_t size);
    void clearCache();

    std::vector<MemoryPool> pools_;
    std::unordered_map<void*, MemoryBlock*> blockMap_;
    std::mutex mutex_;
    std::atomic<size_t> memoryLimit_{MAX_POOL_SIZE};
    MemoryStats stats_;
    bool initialized_{false};
};

// Реализация шаблонных методов
template<typename T>
void MemoryManager::fill(T* ptr, const T& value, size_t count) {
    if (!ptr || count == 0) return;

#ifdef __x86_64__
    if (count >= 4 && std::is_trivially_copyable<T>::value) {
        __m128i val = _mm_set1_epi32(static_cast<int>(value));
        for (size_t i = 0; i < count; i += 4) {
            _mm_store_si128(reinterpret_cast<__m128i*>(ptr + i), val);
        }
    } else
#elif defined(__aarch64__)
    if (count >= 4 && std::is_trivially_copyable<T>::value) {
        uint32x4_t val = vdupq_n_u32(static_cast<uint32_t>(value));
        for (size_t i = 0; i < count; i += 4) {
            vst1q_u32(reinterpret_cast<uint32_t*>(ptr + i), val);
        }
    } else
#endif
    {
        std::fill(ptr, ptr + count, value);
    }
}

template<typename T>
void MemoryManager::copy(T* dst, const T* src, size_t count) {
    if (!dst || !src || count == 0) return;

#ifdef __x86_64__
    if (count >= 4 && std::is_trivially_copyable<T>::value) {
        for (size_t i = 0; i < count; i += 4) {
            __m128i val = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + i));
            _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + i), val);
        }
    } else
#elif defined(__aarch64__)
    if (count >= 4 && std::is_trivially_copyable<T>::value) {
        for (size_t i = 0; i < count; i += 4) {
            uint32x4_t val = vld1q_u32(reinterpret_cast<const uint32_t*>(src + i));
            vst1q_u32(reinterpret_cast<uint32_t*>(dst + i), val);
        }
    } else
#endif
    {
        std::copy(src, src + count, dst);
    }
}

} // namespace memory 