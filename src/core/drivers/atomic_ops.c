#include "core/drivers/atomic_ops.h"

#if defined(__x86_64__)
#include <immintrin.h>
#endif

// Атомарные операции для x86-64
#if defined(__x86_64__)
int64_t core_atomic_add_64(int64_t* ptr, int64_t value) {
    return __sync_add_and_fetch(ptr, value);
}

int64_t core_atomic_sub_64(int64_t* ptr, int64_t value) {
    return __sync_sub_and_fetch(ptr, value);
}

int64_t core_atomic_cas_64(int64_t* ptr, int64_t old_val, int64_t new_val) {
    return __sync_val_compare_and_swap(ptr, old_val, new_val);
}

int64_t core_atomic_xchg_64(int64_t* ptr, int64_t new_val) {
    return __sync_lock_test_and_set(ptr, new_val);
}

void core_atomic_store_64(int64_t* ptr, int64_t value) {
    __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST);
}

int64_t core_atomic_load_64(const int64_t* ptr) {
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

void core_memory_barrier() {
    __asm__ __volatile__("mfence" ::: "memory");
}

void core_memory_barrier_acquire() {
    __asm__ __volatile__("" ::: "memory");
}

void core_memory_barrier_release() {
    __asm__ __volatile__("" ::: "memory");
}

int core_atomic_test_and_set(uint8_t* ptr) {
    return __sync_lock_test_and_set(ptr, 1);
}

void core_atomic_clear(uint8_t* ptr) {
    __sync_lock_release(ptr);
}

// Атомарные операции для ARM
#elif defined(__aarch64__)
int64_t core_atomic_add_64(int64_t* ptr, int64_t value) {
    int64_t old_val;
    __asm__ __volatile__(
        "1: ldaxr %0, [%1]\n"
        "add %0, %0, %2\n"
        "stlxr w8, %0, [%1]\n"
        "cbnz w8, 1b\n"
        : "=&r" (old_val)
        : "r" (ptr), "r" (value)
        : "w8", "memory"
    );
    return old_val + value;
}

int64_t core_atomic_sub_64(int64_t* ptr, int64_t value) {
    int64_t old_val;
    __asm__ __volatile__(
        "1: ldaxr %0, [%1]\n"
        "sub %0, %0, %2\n"
        "stlxr w8, %0, [%1]\n"
        "cbnz w8, 1b\n"
        : "=&r" (old_val)
        : "r" (ptr), "r" (value)
        : "w8", "memory"
    );
    return old_val - value;
}

int64_t core_atomic_cas_64(int64_t* ptr, int64_t old_val, int64_t new_val) {
    int64_t result;
    __asm__ __volatile__(
        "1: ldaxr %0, [%1]\n"
        "cmp %0, %2\n"
        "b.ne 2f\n"
        "stlxr w8, %3, [%1]\n"
        "cbnz w8, 1b\n"
        "2:\n"
        : "=&r" (result)
        : "r" (ptr), "r" (old_val), "r" (new_val)
        : "w8", "memory"
    );
    return result;
}

int64_t core_atomic_xchg_64(int64_t* ptr, int64_t new_val) {
    int64_t old_val;
    __asm__ __volatile__(
        "1: ldaxr %0, [%1]\n"
        "stlxr w8, %2, [%1]\n"
        "cbnz w8, 1b\n"
        : "=&r" (old_val)
        : "r" (ptr), "r" (new_val)
        : "w8", "memory"
    );
    return old_val;
}

void core_atomic_store_64(int64_t* ptr, int64_t value) {
    __asm__ __volatile__(
        "stlr %0, [%1]\n"
        : : "r" (value), "r" (ptr) : "memory"
    );
}

int64_t core_atomic_load_64(const int64_t* ptr) {
    int64_t value;
    __asm__ __volatile__(
        "ldar %0, [%1]\n"
        : "=r" (value) : "r" (ptr) : "memory"
    );
    return value;
}

void core_memory_barrier() {
    __asm__ __volatile__("dmb ish" ::: "memory");
}

void core_memory_barrier_acquire() {
    __asm__ __volatile__("dmb ishld" ::: "memory");
}

void core_memory_barrier_release() {
    __asm__ __volatile__("dmb ishst" ::: "memory");
}

int core_atomic_test_and_set(uint8_t* ptr) {
    int result;
    __asm__ __volatile__(
        "1: ldaxrb %w0, [%1]\n"
        "cbnz %w0, 2f\n"
        "stlxrb w8, %w2, [%1]\n"
        "cbnz w8, 1b\n"
        "2:\n"
        : "=&r" (result)
        : "r" (ptr), "r" (1)
        : "w8", "memory"
    );
    return result;
}

void core_atomic_clear(uint8_t* ptr) {
    __asm__ __volatile__(
        "stlrb wzr, [%0]\n"
        : : "r" (ptr) : "memory"
    );
}

// Fallback для других архитектур
#else
int64_t core_atomic_add_64(int64_t* ptr, int64_t value) {
    int64_t old_val = *ptr;
    *ptr += value;
    return old_val + value;
}

int64_t core_atomic_sub_64(int64_t* ptr, int64_t value) {
    int64_t old_val = *ptr;
    *ptr -= value;
    return old_val - value;
}

int64_t core_atomic_cas_64(int64_t* ptr, int64_t old_val, int64_t new_val) {
    int64_t current = *ptr;
    if (current == old_val) {
        *ptr = new_val;
    }
    return current;
}

int64_t core_atomic_xchg_64(int64_t* ptr, int64_t new_val) {
    int64_t old_val = *ptr;
    *ptr = new_val;
    return old_val;
}

void core_atomic_store_64(int64_t* ptr, int64_t value) {
    *ptr = value;
}

int64_t core_atomic_load_64(const int64_t* ptr) {
    return *ptr;
}

void core_memory_barrier() {
    // Fallback implementation
}

void core_memory_barrier_acquire() {
    // Fallback implementation
}

void core_memory_barrier_release() {
    // Fallback implementation
}

int core_atomic_test_and_set(uint8_t* ptr) {
    int old_val = *ptr;
    *ptr = 1;
    return old_val;
}

void core_atomic_clear(uint8_t* ptr) {
    *ptr = 0;
}
#endif 