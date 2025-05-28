#include "architecture.h"
#include <cpuid.h>
#include <cstring>
#include <memory>
#include <stdexcept>

namespace cloud {

// Реализация архитектурно-зависимых функций
namespace arch {

// Определение архитектуры процессора
Architecture detect_architecture() {
    unsigned int eax, ebx, ecx, edx;
    
    // Получение информации о процессоре
    if (!__get_cpuid(0, &eax, &ebx, &ecx, &edx)) {
        throw std::runtime_error("Failed to get CPU information");
    }
    
    // Проверка на ARM
    if (memcmp(&ebx, "ARM", 3) == 0) {
        return Architecture::ARM64;
    }
    
    // Проверка на x86
    if (memcmp(&ebx, "GenuineIntel", 12) == 0 || 
        memcmp(&ebx, "AuthenticAMD", 12) == 0) {
        return Architecture::X86_64;
    }
    
    return Architecture::UNKNOWN;
}

// Получение размера кэш-линии
size_t get_cache_line_size() {
    unsigned int eax, ebx, ecx, edx;
    
    if (!__get_cpuid(0x80000006, &eax, &ebx, &ecx, &edx)) {
        return 64; // Возвращаем значение по умолчанию
    }
    
    return ecx & 0xFF;
}

// Проверка поддержки SIMD инструкций
bool has_simd_support() {
    unsigned int eax, ebx, ecx, edx;
    
    if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        return false;
    }
    
    // Проверка поддержки SSE4.2
    return (ecx & (1 << 20)) != 0;
}

// Проверка поддержки AVX
bool has_avx_support() {
    unsigned int eax, ebx, ecx, edx;
    
    if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        return false;
    }
    
    // Проверка поддержки AVX
    return (ecx & (1 << 28)) != 0;
}

// Получение количества ядер процессора
unsigned int get_cpu_cores() {
    unsigned int eax, ebx, ecx, edx;
    
    if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        return 1; // Возвращаем значение по умолчанию
    }
    
    return ((ebx >> 16) & 0xFF);
}

// Получение размера страницы памяти
size_t get_page_size() {
    return sysconf(_SC_PAGESIZE);
}

// Выравнивание адреса по границе кэш-линии
void* align_to_cache_line(void* ptr) {
    size_t alignment = get_cache_line_size();
    size_t addr = reinterpret_cast<size_t>(ptr);
    size_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
    return reinterpret_cast<void*>(aligned_addr);
}

// Проверка поддержки NUMA
bool has_numa_support() {
    return numa_available() >= 0;
}

// Получение количества NUMA узлов
int get_numa_node_count() {
    if (!has_numa_support()) {
        return 1;
    }
    return numa_num_configured_nodes();
}

// Привязка потока к конкретному NUMA узлу
void bind_to_numa_node(int node_id) {
    if (!has_numa_support()) {
        return;
    }
    
    if (node_id < 0 || node_id >= get_numa_node_count()) {
        throw std::invalid_argument("Invalid NUMA node ID");
    }
    
    struct bitmask* mask = numa_allocate_cpumask();
    numa_bitmask_setbit(mask, node_id);
    numa_bind(mask);
    numa_free_cpumask(mask);
}

// Получение информации о памяти NUMA узла
NumaMemoryInfo get_numa_memory_info(int node_id) {
    if (!has_numa_support()) {
        return NumaMemoryInfo{0, 0, 0};
    }
    
    if (node_id < 0 || node_id >= get_numa_node_count()) {
        throw std::invalid_argument("Invalid NUMA node ID");
    }
    
    NumaMemoryInfo info;
    info.total = numa_node_size64(node_id, &info.free);
    info.used = info.total - info.free;
    return info;
}

// Оптимизация памяти для конкретной архитектуры
void optimize_memory_allocation(void* ptr, size_t size) {
    size_t page_size = get_page_size();
    size_t cache_line = get_cache_line_size();
    
    // Выравнивание по границе страницы
    void* aligned_ptr = reinterpret_cast<void*>(
        (reinterpret_cast<size_t>(ptr) + page_size - 1) & ~(page_size - 1)
    );
    
    // Установка политики доступа к памяти
    if (has_numa_support()) {
        int node_id = numa_node_of_cpu(sched_getcpu());
        numa_tonode_memory(aligned_ptr, size, node_id);
    }
    
    // Предварительная загрузка в кэш
    for (size_t i = 0; i < size; i += cache_line) {
        __builtin_prefetch(
            reinterpret_cast<char*>(aligned_ptr) + i,
            1, // Write
            3  // High temporal locality
        );
    }
}

} // namespace arch
} // namespace cloud 