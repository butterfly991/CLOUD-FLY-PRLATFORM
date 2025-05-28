#include "core/drivers/memory_ops.h"

#if defined(__AVX2__)
#include <immintrin.h>
#elif defined(__ARM_NEON)
#include <arm_neon.h>
#endif

// Оптимизированные операции с памятью
void* core_memcpy_aligned(void* dest, const void* src, size_t n) {
#if defined(__AVX2__)
    size_t i = 0;
    // Копируем по 32 байта (AVX2)
    for (; i + 32 <= n; i += 32) {
        __m256i data = _mm256_load_si256((__m256i*)((char*)src + i));
        _mm256_store_si256((__m256i*)((char*)dest + i), data);
    }
    // Копируем оставшиеся байты
    for (; i < n; ++i) {
        ((char*)dest)[i] = ((char*)src)[i];
    }
#elif defined(__ARM_NEON)
    size_t i = 0;
    // Копируем по 16 байт (NEON)
    for (; i + 16 <= n; i += 16) {
        uint8x16_t data = vld1q_u8((uint8_t*)((char*)src + i));
        vst1q_u8((uint8_t*)((char*)dest + i), data);
    }
    // Копируем оставшиеся байты
    for (; i < n; ++i) {
        ((char*)dest)[i] = ((char*)src)[i];
    }
#else
    // Стандартное копирование
    for (size_t i = 0; i < n; ++i) {
        ((char*)dest)[i] = ((char*)src)[i];
    }
#endif
    return dest;
}

void* core_memset_aligned(void* dest, int val, size_t n) {
#if defined(__AVX2__)
    size_t i = 0;
    __m256i data = _mm256_set1_epi8(val);
    // Заполняем по 32 байта (AVX2)
    for (; i + 32 <= n; i += 32) {
        _mm256_store_si256((__m256i*)((char*)dest + i), data);
    }
    // Заполняем оставшиеся байты
    for (; i < n; ++i) {
        ((char*)dest)[i] = val;
    }
#elif defined(__ARM_NEON)
    size_t i = 0;
    uint8x16_t data = vdupq_n_u8(val);
    // Заполняем по 16 байт (NEON)
    for (; i + 16 <= n; i += 16) {
        vst1q_u8((uint8_t*)((char*)dest + i), data);
    }
    // Заполняем оставшиеся байты
    for (; i < n; ++i) {
        ((char*)dest)[i] = val;
    }
#else
    // Стандартное заполнение
    for (size_t i = 0; i < n; ++i) {
        ((char*)dest)[i] = val;
    }
#endif
    return dest;
}

void core_memzero_aligned(void* dest, size_t n) {
    core_memset_aligned(dest, 0, n);
}

// Операции с выравниванием
void* core_align_ptr(void* ptr, size_t alignment) {
    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t aligned = (addr + alignment - 1) & ~(alignment - 1);
    return (void*)aligned;
}

size_t core_get_alignment_offset(const void* ptr, size_t alignment) {
    uintptr_t addr = (uintptr_t)ptr;
    return (alignment - (addr & (alignment - 1))) & (alignment - 1);
}

// Операции с кэшем
void core_prefetch(const void* ptr) {
#if defined(__AVX2__)
    _mm_prefetch((const char*)ptr, _MM_HINT_T0);
#elif defined(__ARM_NEON)
    __builtin_prefetch(ptr, 0, 3);
#else
    // Fallback implementation
#endif
}

void core_prefetch_write(const void* ptr) {
#if defined(__AVX2__)
    _mm_prefetch((const char*)ptr, _MM_HINT_T1);
#elif defined(__ARM_NEON)
    __builtin_prefetch(ptr, 1, 3);
#else
    // Fallback implementation
#endif
}

void core_flush_cache_line(const void* ptr) {
#if defined(__AVX2__)
    _mm_clflush(ptr);
#elif defined(__ARM_NEON)
    // ARM не имеет прямой эквивалент clflush
    // Используем барьер памяти
    __asm__ __volatile__("dmb ish" ::: "memory");
#else
    // Fallback implementation
#endif
}

void core_invalidate_cache_line(const void* ptr) {
#if defined(__AVX2__)
    _mm_clflush(ptr);
#elif defined(__ARM_NEON)
    // ARM не имеет прямой эквивалент clflush
    // Используем барьер памяти
    __asm__ __volatile__("dmb ish" ::: "memory");
#else
    // Fallback implementation
#endif
}

// Операции с памятью с барьерами
void core_memory_barrier_load() {
#if defined(__AVX2__)
    _mm_lfence();
#elif defined(__ARM_NEON)
    __asm__ __volatile__("dmb ishld" ::: "memory");
#else
    // Fallback implementation
#endif
}

void core_memory_barrier_store() {
#if defined(__AVX2__)
    _mm_sfence();
#elif defined(__ARM_NEON)
    __asm__ __volatile__("dmb ishst" ::: "memory");
#else
    // Fallback implementation
#endif
}

void core_memory_barrier_full() {
#if defined(__AVX2__)
    _mm_mfence();
#elif defined(__ARM_NEON)
    __asm__ __volatile__("dmb ish" ::: "memory");
#else
    // Fallback implementation
#endif
} 