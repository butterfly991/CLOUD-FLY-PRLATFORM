#include "core/drivers/cpu_info.h"
#if defined(__x86_64__)
#include <cpuid.h>
#endif

int core_cpu_arch() {
#if defined(__x86_64__)
    return CORE_CPU_X86_64;
#elif defined(__aarch64__)
    return CORE_CPU_ARM64;
#else
    return 0;
#endif
}

int core_cpu_has_avx2() {
#if defined(__x86_64__)
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
        return (ebx & (1 << 5)) != 0; // AVX2 bit
    }
    return 0;
#else
    return 0;
#endif
}

int core_cpu_has_neon() {
#if defined(__aarch64__)
    return 1;
#else
    return 0;
#endif
} 