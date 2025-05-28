#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

// Интерфейс аллокатора ядра
void* core_malloc(size_t size);
void  core_free(void* ptr);
void* core_calloc(size_t n, size_t size);
void* core_realloc(void* ptr, size_t size);

// Диагностика и статистика
size_t core_memory_used();
size_t core_memory_available();

#ifdef __cplusplus
}
#endif 