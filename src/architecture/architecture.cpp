#include "architecture.h"
#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <mutex>
#include <fstream>
#include <sstream>

#ifdef __linux__
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <cpuid.h>
#include <numa.h>
#elif defined(_WIN32)
#include <windows.h>
#include <intrin.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#endif

namespace arch {

// Singleton instance
ArchitectureOptimizer& ArchitectureOptimizer::getInstance() {
    static ArchitectureOptimizer instance;
    return instance;
}

void ArchitectureOptimizer::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) return;

    // Определяем архитектуру
    systemInfo_.architecture = detectArchitecture();
    systemInfo_.osType = detectOS();

    // Получаем информацию о CPU
    systemInfo_.cpu = getCPUInfo();

    // Получаем информацию о памяти
    systemInfo_.memory = getMemoryInfo();

    // Проверяем виртуализацию
    systemInfo_.isVirtualized = isVirtualized();
    systemInfo_.isContainerized = isContainerized();

    initialized_ = true;
}

SystemInfo ArchitectureOptimizer::getSystemInfo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return systemInfo_;
}

// Определение архитектуры
ArchitectureType detectArchitecture() {
#if defined(__x86_64__)
    return ArchitectureType::X86_64;
#elif defined(__aarch64__)
    return ArchitectureType::ARM64;
#elif defined(__riscv)
    return ArchitectureType::RISCV;
#elif defined(__powerpc__)
    return ArchitectureType::POWERPC;
#elif defined(__mips__)
    return ArchitectureType::MIPS;
#else
    return ArchitectureType::UNKNOWN;
#endif
}

// Определение ОС
OSType detectOS() {
#if defined(__linux__)
    return OSType::Linux;
#elif defined(_WIN32)
    return OSType::Windows;
#elif defined(__APPLE__)
    return OSType::macOS;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    return OSType::BSD;
#elif defined(__ANDROID__)
    return OSType::Android;
#elif defined(__APPLE__) && defined(__IPHONE_OS_VERSION_MIN_REQUIRED)
    return OSType::iOS;
#else
    return OSType::UNKNOWN;
#endif
}

// Получение информации о CPU
CPUInfo getCPUInfo() {
    CPUInfo info;
    info.cores = std::thread::hardware_concurrency();
    info.threads = std::thread::hardware_concurrency();
    info.cacheLineSize = getCacheLineSize();

#ifdef __linux__
    // Linux-specific CPU info
    struct utsname uts;
    uname(&uts);
    info.vendor = uts.machine;
    info.model = uts.nodename;

    // Читаем информацию о CPU из /proc/cpuinfo
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    while (std::getline(cpuinfo, line)) {
        if (line.find("model name") != std::string::npos) {
            info.model = line.substr(line.find(":") + 2);
        }
    }

    // Проверяем поддержку инструкций
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        info.hasAVX = (ecx & bit_AVX) != 0;
        info.hasAVX2 = (ebx & bit_AVX2) != 0;
        info.hasAVX512 = (ebx & bit_AVX512F) != 0;
        info.hasSIMD = (edx & bit_SSE) != 0;
    }

    // Получаем размеры кэша
    std::ifstream cacheinfo("/sys/devices/system/cpu/cpu0/cache/index0/size");
    if (cacheinfo.is_open()) {
        std::string size;
        std::getline(cacheinfo, size);
        info.l1CacheSize = std::stoi(size);
    }

#elif defined(_WIN32)
    // Windows-specific CPU info
    char vendor[13];
    int cpuInfo[4];
    __cpuid(cpuInfo, 0);
    memcpy(vendor, &cpuInfo[1], 4);
    memcpy(vendor + 4, &cpuInfo[3], 4);
    memcpy(vendor + 8, &cpuInfo[2], 4);
    vendor[12] = '\0';
    info.vendor = vendor;

    // Получаем информацию о модели
    char brand[0x40];
    unsigned int CPUInfo[4] = {0};
    for (unsigned int i = 0x80000002; i <= 0x80000004; i++) {
        __cpuid((int*)CPUInfo, i);
        memcpy(brand + (i - 0x80000002) * 16, CPUInfo, sizeof(CPUInfo));
    }
    info.model = brand;

    // Проверяем поддержку инструкций
    int cpuInfo2[4];
    __cpuid(cpuInfo2, 1);
    info.hasAVX = (cpuInfo2[2] & (1 << 28)) != 0;
    info.hasAVX2 = (cpuInfo2[7] & (1 << 5)) != 0;
    info.hasAVX512 = (cpuInfo2[7] & (1 << 16)) != 0;
    info.hasSIMD = (cpuInfo2[3] & (1 << 25)) != 0;

#elif defined(__APPLE__)
    // macOS-specific CPU info
    char brand[256];
    size_t size = sizeof(brand);
    if (sysctlbyname("machdep.cpu.brand_string", &brand, &size, nullptr, 0) == 0) {
        info.model = brand;
    }

    // Получаем информацию о вендоре
    char vendor[256];
    size = sizeof(vendor);
    if (sysctlbyname("machdep.cpu.vendor", &vendor, &size, nullptr, 0) == 0) {
        info.vendor = vendor;
    }

    // Проверяем поддержку инструкций
    int hasAVX = 0;
    size = sizeof(hasAVX);
    if (sysctlbyname("machdep.cpu.features", &hasAVX, &size, nullptr, 0) == 0) {
        info.hasAVX = (hasAVX & 0x100000) != 0;
    }
#endif

    // Общие проверки
    info.hasNUMA = hasNUMASupport();
    info.hasHyperThreading = info.threads > info.cores;
    info.supportedInstructions = getSupportedInstructions();

    return info;
}

// Получение информации о памяти
MemoryInfo getMemoryInfo() {
    MemoryInfo info;
    info.pageSize = getPageSize();

#ifdef __linux__
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        info.totalPhysical = si.totalram * si.mem_unit;
        info.availablePhysical = si.freeram * si.mem_unit;
    }

    // Получаем информацию о виртуальной памяти
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    while (std::getline(meminfo, line)) {
        if (line.find("SwapTotal:") != std::string::npos) {
            std::stringstream ss(line.substr(line.find(":") + 1));
            size_t total;
            ss >> total;
            info.totalVirtual = total * 1024;
        }
    }

    // NUMA информация
    if (hasNUMASupport()) {
        info.numaNodeCount = getNUMANodeCount();
        for (int i = 0; i < info.numaNodeCount; i++) {
            info.numaNodeSizes.push_back(getNUMANodeSize(i));
        }
    }

#elif defined(_WIN32)
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    info.totalPhysical = memInfo.ullTotalPhys;
    info.availablePhysical = memInfo.ullAvailPhys;
    info.totalVirtual = memInfo.ullTotalVirtual;
    info.availableVirtual = memInfo.ullAvailVirtual;

#elif defined(__APPLE__)
    vm_size_t pageSize;
    host_page_size(mach_host_self(), &pageSize);
    
    vm_statistics64_data_t vmStats;
    mach_msg_type_number_t infoCount = sizeof(vmStats) / sizeof(natural_t);
    host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&vmStats, &infoCount);
    
    info.totalPhysical = (vmStats.free_count + vmStats.active_count + vmStats.inactive_count + vmStats.wire_count) * pageSize;
    info.availablePhysical = vmStats.free_count * pageSize;
#endif

    return info;
}

// Проверка виртуализации
bool isVirtualized() {
#ifdef __linux__
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    while (std::getline(cpuinfo, line)) {
        if (line.find("hypervisor") != std::string::npos) {
            return true;
        }
    }
#elif defined(_WIN32)
    int cpuInfo[4];
    __cpuid(cpuInfo, 1);
    return (cpuInfo[2] & (1 << 31)) != 0;
#elif defined(__APPLE__)
    int isVM = 0;
    size_t size = sizeof(isVM);
    if (sysctlbyname("machdep.cpu.features", &isVM, &size, nullptr, 0) == 0) {
        return (isVM & 0x80000000) != 0;
    }
#endif
    return false;
}

// Проверка контейнеризации
bool isContainerized() {
#ifdef __linux__
    std::ifstream cgroup("/proc/1/cgroup");
    if (!cgroup.is_open()) return false;
    
    std::string line;
    while (std::getline(cgroup, line)) {
        if (line.find("docker") != std::string::npos ||
            line.find("kubepods") != std::string::npos) {
            return true;
        }
    }
#endif
    return false;
}

// Остальные реализации методов...
bool hasNUMASupport() {
#ifdef __linux__
    return numa_available() >= 0;
#else
    return false;
#endif
}

int getNUMANodeCount() {
#ifdef __linux__
    if (hasNUMASupport()) {
        return numa_num_configured_nodes();
    }
#endif
    return 1;
}

size_t getNUMANodeSize(int node) {
#ifdef __linux__
    if (hasNUMASupport() && node >= 0 && node < getNUMANodeCount()) {
        return numa_node_size64(node, nullptr);
    }
#endif
    return 0;
}

int getCacheLineSize() {
#ifdef __linux__
    long sz = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    if (sz > 0) return static_cast<int>(sz);
#elif defined(_WIN32)
    int cpuInfo[4];
    __cpuid(cpuInfo, 0x80000006);
    return cpuInfo[2] & 0xFF;
#elif defined(__APPLE__)
    int lineSize = 0;
    size_t size = sizeof(lineSize);
    if (sysctlbyname("machdep.cpu.cache.linesize", &lineSize, &size, nullptr, 0) == 0) {
        return lineSize;
    }
#endif
    return 64; // Default fallback
}

std::vector<std::string> getSupportedInstructions() {
    std::vector<std::string> instructions;
#if defined(__x86_64__)
    instructions.push_back("SSE");
    instructions.push_back("SSE2");
    instructions.push_back("SSE3");
    instructions.push_back("SSSE3");
    instructions.push_back("SSE4.1");
    instructions.push_back("SSE4.2");
    instructions.push_back("AVX");
    instructions.push_back("AVX2");
    instructions.push_back("AVX512");
#elif defined(__aarch64__)
    instructions.push_back("NEON");
    instructions.push_back("ASIMD");
#endif
    return instructions;
}

size_t getPageSize() {
#ifdef __linux__
    return sysconf(_SC_PAGESIZE);
#else
    return 4096;
#endif
}

void initializeOptimizations() {
    std::cout << "[ARCH] Инициализация архитектурных оптимизаций...\n";
    // Здесь можно инициализировать SIMD, NUMA и др.
}

int getPhysicalCores() {
    return std::thread::hardware_concurrency();
}

int getLogicalThreads() {
    return std::thread::hardware_concurrency();
}

} // namespace arch 