#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace arch {

// Тип архитектуры
enum class ArchitectureType {
    X86_64,
    ARM64,
    RISCV,
    POWERPC,
    MIPS,
    UNKNOWN
};

// Тип операционной системы
enum class OSType {
    Linux,
    Windows,
    macOS,
    BSD,
    Android,
    iOS,
    UNKNOWN
};

// Информация о CPU
struct CPUInfo {
    std::string vendor;
    std::string model;
    int cores;
    int threads;
    int cacheLineSize;
    int l1CacheSize;
    int l2CacheSize;
    int l3CacheSize;
    bool hasSIMD;
    bool hasAVX;
    bool hasAVX2;
    bool hasAVX512;
    bool hasNEON;
    bool hasNUMA;
    bool hasHyperThreading;
    bool hasVirtualization;
    std::vector<std::string> supportedInstructions;
};

// Информация о памяти
struct MemoryInfo {
    size_t totalPhysical;
    size_t availablePhysical;
    size_t totalVirtual;
    size_t availableVirtual;
    size_t pageSize;
    int numaNodeCount;
    std::vector<size_t> numaNodeSizes;
};

// Информация о системе
struct SystemInfo {
    ArchitectureType architecture;
    OSType osType;
    std::string osVersion;
    std::string osName;
    CPUInfo cpu;
    MemoryInfo memory;
    bool isVirtualized;
    bool isContainerized;
};

// Класс для работы с архитектурными оптимизациями
class ArchitectureOptimizer {
public:
    static ArchitectureOptimizer& getInstance();
    
    // Инициализация оптимизаций
    void initialize();
    
    // Получение информации о системе
    SystemInfo getSystemInfo() const;
    
    // Оптимизации для разных архитектур
    void optimizeForArchitecture(ArchitectureType arch);
    void optimizeForCPU(const CPUInfo& cpu);
    void optimizeForMemory(const MemoryInfo& mem);
    
    // Проверка поддержки инструкций
    bool supportsInstruction(const std::string& instruction) const;
    bool supportsSIMD() const;
    bool supportsAVX() const;
    bool supportsNEON() const;
    
    // Работа с NUMA
    bool hasNUMASupport() const;
    int getNUMANodeCount() const;
    size_t getNUMANodeSize(int node) const;
    
    // Работа с кэшем
    int getCacheLineSize() const;
    int getL1CacheSize() const;
    int getL2CacheSize() const;
    int getL3CacheSize() const;
    
    // Работа с потоками
    int getPhysicalCores() const;
    int getLogicalThreads() const;
    bool hasHyperThreading() const;
    
    // Работа с памятью
    size_t getPageSize() const;
    size_t getTotalPhysicalMemory() const;
    size_t getAvailablePhysicalMemory() const;
    
    // Виртуализация
    bool isVirtualized() const;
    bool isContainerized() const;
    
private:
    ArchitectureOptimizer() = default;
    ~ArchitectureOptimizer() = default;
    ArchitectureOptimizer(const ArchitectureOptimizer&) = delete;
    ArchitectureOptimizer& operator=(const ArchitectureOptimizer&) = delete;
    
    SystemInfo systemInfo_;
    bool initialized_ = false;
    mutable std::mutex mutex_;
};

// Глобальные функции для удобства использования
ArchitectureType detectArchitecture();
OSType detectOS();
CPUInfo getCPUInfo();
MemoryInfo getMemoryInfo();
SystemInfo getSystemInfo();
bool hasNUMASupport();
int getNUMANodeCount();
void initializeOptimizations();
std::vector<std::string> getSupportedInstructions();
int getCacheLineSize();
int getPhysicalCores();
int getLogicalThreads();
size_t getPageSize();
bool isVirtualized();
bool isContainerized();

} // namespace arch 