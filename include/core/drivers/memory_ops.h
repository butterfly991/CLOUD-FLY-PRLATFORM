#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

// Оптимизированные операции с памятью
void* core_memcpy_aligned(void* dest, const void* src, size_t n);
void* core_memset_aligned(void* dest, int val, size_t n);
void core_memzero_aligned(void* dest, size_t n);

// Операции с выравниванием
void* core_align_ptr(void* ptr, size_t alignment);
size_t core_get_alignment_offset(const void* ptr, size_t alignment);

// Операции с кэшем
void core_prefetch(const void* ptr);
void core_prefetch_write(const void* ptr);
void core_flush_cache_line(const void* ptr);
void core_invalidate_cache_line(const void* ptr);

// Операции с памятью с барьерами
void core_memory_barrier_load();
void core_memory_barrier_store();
void core_memory_barrier_full();

#ifdef __cplusplus
}
#endif 