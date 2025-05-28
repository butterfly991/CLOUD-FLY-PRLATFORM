#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Атомарные операции с оптимизациями для x86-64 и ARM
int64_t core_atomic_add_64(int64_t* ptr, int64_t value);
int64_t core_atomic_sub_64(int64_t* ptr, int64_t value);
int64_t core_atomic_cas_64(int64_t* ptr, int64_t old_val, int64_t new_val);
int64_t core_atomic_xchg_64(int64_t* ptr, int64_t new_val);

// Атомарные операции с памятью
void core_atomic_store_64(int64_t* ptr, int64_t value);
int64_t core_atomic_load_64(const int64_t* ptr);

// Атомарные операции с барьерами памяти
void core_memory_barrier();
void core_memory_barrier_acquire();
void core_memory_barrier_release();

// Атомарные операции с флагами
int core_atomic_test_and_set(uint8_t* ptr);
void core_atomic_clear(uint8_t* ptr);

#ifdef __cplusplus
}
#endif 