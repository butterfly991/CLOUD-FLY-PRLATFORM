#include "memory_manager.h"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <chrono>
#include <thread>
#include <random>
#include <sstream>
#include <iomanip>

#ifdef __linux__
#include <sys/mman.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/vm_statistics.h>
#endif

namespace memory {

// Singleton instance
MemoryManager& MemoryManager::getInstance() {
    static MemoryManager instance;
    return instance;
}

MemoryManager::MemoryManager() {
    initialize();
}

MemoryManager::~MemoryManager() {
    shutdown();
}

void MemoryManager::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) return;

    try {
        initializePools();
        initialized_ = true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize MemoryManager: " << e.what() << std::endl;
        cleanupPools();
        throw;
    }
}

void MemoryManager::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_) return;

    cleanupPools();
    initialized_ = false;
}

void MemoryManager::setMemoryLimit(size_t limit) {
    std::lock_guard<std::mutex> lock(mutex_);
    memoryLimit_ = limit;
}

void* MemoryManager::allocate(size_t size, size_t alignment) {
    if (size == 0) return nullptr;
    if (size > MAX_BLOCK_SIZE) {
        return allocateAligned(size, alignment);
    }

    std::lock_guard<std::mutex> lock(mutex_);
    
    // Проверяем кэш
    void* cached = getFromCache(size);
    if (cached) {
        ++stats_.cacheHits;
        return cached;
    }
    ++stats_.cacheMisses;

    // Ищем свободный блок
    MemoryBlock* block = findFreeBlock(size, alignment);
    if (!block) {
        defragment();
        block = findFreeBlock(size, alignment);
    }

    if (!block) {
        return allocateAligned(size, alignment);
    }

    block->isUsed = true;
    block->lastAccess = std::chrono::steady_clock::now();
    blockMap_[block->ptr] = block;

    updateStats(size, 0);
    return block->ptr;
}

void MemoryManager::deallocate(void* ptr) {
    if (!ptr) return;

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = blockMap_.find(ptr);
    if (it != blockMap_.end()) {
        MemoryBlock* block = it->second;
        block->isUsed = false;
        blockMap_.erase(it);
        updateStats(0, block->size);
        putToCache(ptr, block->size);
    } else {
        deallocateAligned(ptr);
    }
}

void* MemoryManager::reallocate(void* ptr, size_t newSize) {
    if (!ptr) return allocate(newSize);
    if (newSize == 0) {
        deallocate(ptr);
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = blockMap_.find(ptr);
    if (it != blockMap_.end()) {
        MemoryBlock* block = it->second;
        if (newSize <= block->size) {
            return ptr;
        }

        void* newPtr = allocate(newSize);
        if (newPtr) {
            copy(newPtr, ptr, block->size);
            deallocate(ptr);
        }
        return newPtr;
    }

    return allocateAligned(newSize, DEFAULT_ALIGNMENT);
}

void MemoryManager::prefetch(const void* ptr, size_t size) {
#ifdef __x86_64__
    const char* p = static_cast<const char*>(ptr);
    for (size_t i = 0; i < size; i += DEFAULT_PAGE_SIZE) {
        _mm_prefetch(p + i, _MM_HINT_T0);
    }
#elif defined(__aarch64__)
    const char* p = static_cast<const char*>(ptr);
    for (size_t i = 0; i < size; i += DEFAULT_PAGE_SIZE) {
        __builtin_prefetch(p + i, 1, 3);
    }
#endif
}

void MemoryManager::flush(const void* ptr, size_t size) {
#ifdef __linux__
    msync(const_cast<void*>(ptr), size, MS_SYNC);
#elif defined(_WIN32)
    FlushViewOfFile(const_cast<void*>(ptr), size);
#elif defined(__APPLE__)
    msync(const_cast<void*>(ptr), size, MS_SYNC);
#endif
}

void MemoryManager::zero(void* ptr, size_t size) {
    if (!ptr || size == 0) return;

#ifdef __x86_64__
    if (size >= 16) {
        __m128i zero = _mm_setzero_si128();
        for (size_t i = 0; i < size; i += 16) {
            _mm_storeu_si128(reinterpret_cast<__m128i*>(static_cast<char*>(ptr) + i), zero);
        }
    } else
#elif defined(__aarch64__)
    if (size >= 16) {
        uint8x16_t zero = vdupq_n_u8(0);
        for (size_t i = 0; i < size; i += 16) {
            vst1q_u8(static_cast<uint8_t*>(ptr) + i, zero);
        }
    } else
#endif
    {
        std::memset(ptr, 0, size);
    }
}

void MemoryManager::copy(void* dst, const void* src, size_t size) {
    if (!dst || !src || size == 0) return;

#ifdef __x86_64__
    if (size >= 16) {
        for (size_t i = 0; i < size; i += 16) {
            __m128i val = _mm_loadu_si128(reinterpret_cast<const __m128i*>(static_cast<const char*>(src) + i));
            _mm_storeu_si128(reinterpret_cast<__m128i*>(static_cast<char*>(dst) + i), val);
        }
    } else
#elif defined(__aarch64__)
    if (size >= 16) {
        for (size_t i = 0; i < size; i += 16) {
            uint8x16_t val = vld1q_u8(static_cast<const uint8_t*>(src) + i);
            vst1q_u8(static_cast<uint8_t*>(dst) + i, val);
        }
    } else
#endif
    {
        std::memcpy(dst, src, size);
    }
}

void MemoryManager::move(void* dst, const void* src, size_t size) {
    if (!dst || !src || size == 0) return;
    std::memmove(dst, src, size);
}

MemoryStats MemoryManager::getStats() const {
    return stats_;
}

void MemoryManager::resetStats() {
    stats_ = MemoryStats{};
}

size_t MemoryManager::getPageSize() const {
#ifdef __linux__
    return sysconf(_SC_PAGESIZE);
#elif defined(_WIN32)
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return sysInfo.dwPageSize;
#elif defined(__APPLE__)
    return vm_page_size;
#endif
}

size_t MemoryManager::getTotalMemory() const {
#ifdef __linux__
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        return si.totalram * si.mem_unit;
    }
#elif defined(_WIN32)
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        return memInfo.ullTotalPhys;
    }
#elif defined(__APPLE__)
    vm_size_t pageSize;
    vm_statistics64_data_t vmStats;
    mach_msg_type_number_t infoCount = sizeof(vmStats) / sizeof(natural_t);
    if (host_page_size(mach_host_self(), &pageSize) == KERN_SUCCESS &&
        host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&vmStats, &infoCount) == KERN_SUCCESS) {
        return (vmStats.free_count + vmStats.active_count + vmStats.inactive_count + vmStats.wire_count) * pageSize;
    }
#endif
    return 0;
}

size_t MemoryManager::getAvailableMemory() const {
#ifdef __linux__
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        return si.freeram * si.mem_unit;
    }
#elif defined(_WIN32)
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        return memInfo.ullAvailPhys;
    }
#elif defined(__APPLE__)
    vm_size_t pageSize;
    vm_statistics64_data_t vmStats;
    mach_msg_type_number_t infoCount = sizeof(vmStats) / sizeof(natural_t);
    if (host_page_size(mach_host_self(), &pageSize) == KERN_SUCCESS &&
        host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&vmStats, &infoCount) == KERN_SUCCESS) {
        return vmStats.free_count * pageSize;
    }
#endif
    return 0;
}

size_t MemoryManager::getMemoryLimit() const {
    return memoryLimit_;
}

bool MemoryManager::isMemoryAvailable(size_t size) const {
    return size <= getAvailableMemory() && size <= memoryLimit_;
}

void MemoryManager::initializePools() {
    size_t poolSize = MAX_POOL_SIZE / 4;
    for (size_t i = 0; i < 4; ++i) {
        MemoryPool pool;
        pool.totalSize = poolSize;
        pool.usedSize = 0;
        pools_.push_back(std::move(pool));
    }
}

void MemoryManager::cleanupPools() {
    for (auto& pool : pools_) {
        std::lock_guard<std::mutex> lock(pool.mutex);
        for (auto& block : pool.blocks) {
            if (block.ptr) {
                deallocateAligned(block.ptr);
            }
        }
        pool.blocks.clear();
    }
    pools_.clear();
    blockMap_.clear();
}

MemoryManager::MemoryBlock* MemoryManager::findFreeBlock(size_t size, size_t alignment) {
    for (auto& pool : pools_) {
        std::lock_guard<std::mutex> lock(pool.mutex);
        for (auto& block : pool.blocks) {
            if (!block.isUsed && block.size >= size) {
                return &block;
            }
        }
    }
    return nullptr;
}

void MemoryManager::defragment() {
    for (auto& pool : pools_) {
        std::lock_guard<std::mutex> lock(pool.mutex);
        std::sort(pool.blocks.begin(), pool.blocks.end(),
            [](const MemoryBlock& a, const MemoryBlock& b) {
                return a.ptr < b.ptr;
            });
    }
}

void MemoryManager::updateStats(size_t allocated, size_t freed) {
    stats_.totalAllocated += allocated;
    stats_.totalFreed += freed;
    stats_.currentUsage = stats_.totalAllocated - stats_.totalFreed;
    stats_.peakUsage = std::max(stats_.peakUsage, stats_.currentUsage);
    stats_.allocationCount += (allocated > 0);
    stats_.freeCount += (freed > 0);
}

void* MemoryManager::allocateAligned(size_t size, size_t alignment) {
#ifdef __linux__
    void* ptr = nullptr;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return nullptr;
    }
    return ptr;
#elif defined(_WIN32)
    return _aligned_malloc(size, alignment);
#elif defined(__APPLE__)
    void* ptr = nullptr;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return nullptr;
    }
    return ptr;
#endif
}

void MemoryManager::deallocateAligned(void* ptr) {
#ifdef __linux__
    free(ptr);
#elif defined(_WIN32)
    _aligned_free(ptr);
#elif defined(__APPLE__)
    free(ptr);
#endif
}

void* MemoryManager::getFromCache(size_t size) {
    // TODO: Реализовать эффективное кэширование
    return nullptr;
}

void MemoryManager::putToCache(void* ptr, size_t size) {
    // TODO: Реализовать эффективное кэширование
}

void MemoryManager::clearCache() {
    // TODO: Реализовать очистку кэша
}

} // namespace memory 