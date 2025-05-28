#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

// NUMA и affinity API
int core_set_thread_affinity(size_t cpu_id);
int core_get_thread_affinity(size_t* cpu_id);
int core_numa_node_of_cpu(size_t cpu_id);
size_t core_num_numa_nodes();

#ifdef __cplusplus
}
#endif 