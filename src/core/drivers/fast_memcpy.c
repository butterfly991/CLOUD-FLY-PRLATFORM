#include "core/drivers/fast_memcpy.h"

#if defined(__x86_64__)
__attribute__((target("sse2")))
void* core_fast_memcpy(void* dest, const void* src, size_t n) {
    __asm__ __volatile__ (
        "rep movsb"
        : "=D"(dest), "=S"(src), "=c"(n)
        : "0"(dest), "1"(src), "2"(n)
        : "memory"
    );
    return dest;
}
#elif defined(__aarch64__)
void* core_fast_memcpy(void* dest, const void* src, size_t n) {
    // ARM64 NEON оптимизация (упрощённая)
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (size_t i = 0; i < n; ++i) d[i] = s[i];
    return dest;
}
#else
void* core_fast_memcpy(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (size_t i = 0; i < n; ++i) d[i] = s[i];
    return dest;
}
#endif 