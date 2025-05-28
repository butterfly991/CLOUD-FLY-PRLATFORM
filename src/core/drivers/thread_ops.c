#include "core/drivers/thread_ops.h"
#include "core/drivers/atomic_ops.h"
#include "core/drivers/cpu_info.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <sys/time.h>
#endif

// Оптимизированные операции с потоками
int core_thread_create(void (*func)(void*), void* arg) {
#ifdef _WIN32
    HANDLE thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, arg, 0, NULL);
    return (int)(intptr_t)thread;
#else
    pthread_t thread;
    if (pthread_create(&thread, NULL, (void* (*)(void*))func, arg) != 0) {
        return -1;
    }
    return (int)(intptr_t)thread;
#endif
}

void core_thread_join(int thread_id) {
#ifdef _WIN32
    WaitForSingleObject((HANDLE)(intptr_t)thread_id, INFINITE);
    CloseHandle((HANDLE)(intptr_t)thread_id);
#else
    pthread_join((pthread_t)(intptr_t)thread_id, NULL);
#endif
}

void core_thread_yield() {
#ifdef _WIN32
    SwitchToThread();
#else
    sched_yield();
#endif
}

void core_thread_sleep(uint64_t nanoseconds) {
#ifdef _WIN32
    Sleep(nanoseconds / 1000000);
#else
    struct timespec ts;
    ts.tv_sec = nanoseconds / 1000000000;
    ts.tv_nsec = nanoseconds % 1000000000;
    nanosleep(&ts, NULL);
#endif
}

// Операции с приоритетами
int core_thread_set_priority(int thread_id, int priority) {
#ifdef _WIN32
    return SetThreadPriority((HANDLE)(intptr_t)thread_id, priority);
#else
    struct sched_param param;
    param.sched_priority = priority;
    return pthread_setschedparam((pthread_t)(intptr_t)thread_id, SCHED_OTHER, &param);
#endif
}

int core_thread_get_priority(int thread_id) {
#ifdef _WIN32
    return GetThreadPriority((HANDLE)(intptr_t)thread_id);
#else
    struct sched_param param;
    int policy;
    if (pthread_getschedparam((pthread_t)(intptr_t)thread_id, &policy, &param) != 0) {
        return -1;
    }
    return param.sched_priority;
#endif
}

// Операции с affinity
int core_thread_set_affinity(int thread_id, uint64_t mask) {
#ifdef _WIN32
    return SetThreadAffinityMask((HANDLE)(intptr_t)thread_id, mask);
#else
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (int i = 0; i < 64; i++) {
        if (mask & (1ULL << i)) {
            CPU_SET(i, &cpuset);
        }
    }
    return pthread_setaffinity_np((pthread_t)(intptr_t)thread_id, sizeof(cpu_set_t), &cpuset);
#endif
}

uint64_t core_thread_get_affinity(int thread_id) {
#ifdef _WIN32
    DWORD_PTR process_mask, system_mask;
    if (GetProcessAffinityMask(GetCurrentProcess(), &process_mask, &system_mask)) {
        return process_mask;
    }
    return 0;
#else
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    if (pthread_getaffinity_np((pthread_t)(intptr_t)thread_id, sizeof(cpu_set_t), &cpuset) != 0) {
        return 0;
    }
    uint64_t mask = 0;
    for (int i = 0; i < 64; i++) {
        if (CPU_ISSET(i, &cpuset)) {
            mask |= (1ULL << i);
        }
    }
    return mask;
#endif
}

// Операции с NUMA
int core_thread_set_numa_node(int thread_id, int node) {
#ifdef _WIN32
    return SetThreadIdealProcessor((HANDLE)(intptr_t)thread_id, node);
#else
    // Linux не имеет прямого API для установки NUMA node для потока
    // Используем mbind для текущего потока
    unsigned long nodemask = 1ULL << node;
    return syscall(SYS_mbind, 0, 0, MPOL_BIND, &nodemask, sizeof(nodemask), 0);
#endif
}

int core_thread_get_numa_node(int thread_id) {
#ifdef _WIN32
    PROCESSOR_NUMBER proc_num;
    if (GetThreadIdealProcessorEx((HANDLE)(intptr_t)thread_id, &proc_num)) {
        return proc_num.Number;
    }
    return -1;
#else
    // Linux не имеет прямого API для получения NUMA node потока
    // Возвращаем текущий NUMA node
    return 0;
#endif
}

// Операции с синхронизацией
void core_thread_spin_lock(volatile int* lock) {
    while (core_atomic_cas_32(lock, 0, 1) != 0) {
        core_thread_yield();
    }
}

void core_thread_spin_unlock(volatile int* lock) {
    core_atomic_store_32(lock, 0);
}

int core_thread_try_spin_lock(volatile int* lock) {
    return core_atomic_cas_32(lock, 0, 1) == 0;
} 