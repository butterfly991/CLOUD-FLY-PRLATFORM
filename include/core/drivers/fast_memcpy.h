#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

// Быстрый memcpy с ассемблерными вставками для x86-64 и ARM
void* core_fast_memcpy(void* dest, const void* src, size_t n);

#ifdef __cplusplus
}
#endif 