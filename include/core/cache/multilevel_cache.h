#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

// Многоуровневый кэш ядра
#define CORE_CACHE_LEVELS 3

int core_cache_init(size_t l1_size, size_t l2_size, size_t l3_size);
void core_cache_destroy();
int core_cache_put(const void* key, const void* value, size_t size);
int core_cache_get(const void* key, void* value, size_t size);

#ifdef __cplusplus
}
#endif 