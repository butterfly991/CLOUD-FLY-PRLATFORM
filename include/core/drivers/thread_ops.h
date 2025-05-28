#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Оптимизированные операции с потоками
int core_thread_create(void (*func)(void*), void* arg);
void core_thread_join(int thread_id);
void core_thread_yield();
void core_thread_sleep(uint64_t nanoseconds);

// Операции с приоритетами
int core_thread_set_priority(int thread_id, int priority);
int core_thread_get_priority(int thread_id);

// Операции с affinity
int core_thread_set_affinity(int thread_id, uint64_t mask);
uint64_t core_thread_get_affinity(int thread_id);

// Операции с NUMA
int core_thread_set_numa_node(int thread_id, int node);
int core_thread_get_numa_node(int thread_id);

// Операции с синхронизацией
void core_thread_spin_lock(volatile int* lock);
void core_thread_spin_unlock(volatile int* lock);
int core_thread_try_spin_lock(volatile int* lock);

#ifdef __cplusplus
}
#endif 