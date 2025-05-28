#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// CPU feature flags
#define CORE_CPU_X86_64 1
#define CORE_CPU_ARM64  2

int core_cpu_arch(); // Возвращает CORE_CPU_X86_64 или CORE_CPU_ARM64
int core_cpu_has_avx2();
int core_cpu_has_neon();

#ifdef __cplusplus
}
#endif 