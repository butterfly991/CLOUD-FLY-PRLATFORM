#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

// Thread pool API
int core_threadpool_init(size_t num_threads);
void core_threadpool_shutdown();
int core_threadpool_submit(void (*task)(void*), void* arg);
size_t core_threadpool_num_threads();

#ifdef __cplusplus
}
#endif 