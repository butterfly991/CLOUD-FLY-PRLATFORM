#include "adaptive_cache.h"
#include <stdexcept>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <libpmem.h>
#include <numa.h>
#include <numaif.h>
#include <thread>
#include <chrono>

namespace core {
namespace caching {

// NVMBackend Implementation
NVMBackend::NVMBackend(const std::string& path) : pmem_addr_(nullptr), mapped_size_(0) {
    // Open the file
    int fd = open(path.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        throw std::runtime_error("Failed to open NVM file: " + path);
    }

    // Get file size
    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        throw std::runtime_error("Failed to get file size");
    }

    // Map the file
    size_t size = st.st_size;
    if (size == 0) {
        // If file is empty, set a default size
        size = 1024 * 1024 * 1024; // 1GB
        if (ftruncate(fd, size) == -1) {
            close(fd);
            throw std::runtime_error("Failed to set file size");
        }
    }

    // Map the file into memory
    pmem_addr_ = pmem_map_file(path.c_str(), size, PMEM_FILE_CREATE, 0666, &mapped_size_, nullptr);
    if (pmem_addr_ == nullptr) {
        close(fd);
        throw std::runtime_error("Failed to map NVM file");
    }

    close(fd);
}

NVMBackend::~NVMBackend() {
    if (pmem_addr_) {
        pmem_unmap(pmem_addr_, mapped_size_);
    }
}

void NVMBackend::write(const void* data, size_t size, size_t offset) {
    if (offset + size > mapped_size_) {
        throw std::runtime_error("Write operation exceeds mapped memory size");
    }

    // Use pmem_memcpy for persistent memory operations
    pmem_memcpy(static_cast<char*>(pmem_addr_) + offset, data, size, PMEM_F_MEM_NONTEMPORAL);
}

void NVMBackend::read(void* buffer, size_t size, size_t offset) {
    if (offset + size > mapped_size_) {
        throw std::runtime_error("Read operation exceeds mapped memory size");
    }

    // Use pmem_memcpy for persistent memory operations
    pmem_memcpy(buffer, static_cast<char*>(pmem_addr_) + offset, size, PMEM_F_MEM_NONTEMPORAL);
}

// AdaptiveCache Implementation
template<typename Key, typename Value>
void AdaptiveCache<Key, Value>::configure_numa_allocation(int node_count) {
    if (numa_available() == -1) {
        throw std::runtime_error("NUMA is not available on this system");
    }

    // Get NUMA topology information
    struct bitmask* numa_nodes = numa_get_mems_allowed();
    if (!numa_nodes) {
        throw std::runtime_error("Failed to get NUMA nodes information");
    }

    // Calculate memory per node
    size_t memory_per_node = max_size_ * sizeof(CacheNode) / node_count;

    // Allocate memory on each NUMA node
    for (int i = 0; i < node_count; ++i) {
        if (!numa_bitmask_isbitset(numa_nodes, i)) {
            continue; // Skip if node is not available
        }

        void* memory = numa_alloc_onnode(memory_per_node, i);
        if (!memory) {
            numa_bitmask_free(numa_nodes);
            throw std::runtime_error("Failed to allocate NUMA memory on node " + std::to_string(i));
        }

        // Set memory policy for the allocated region
        unsigned long nodemask = 1UL << i;
        if (mbind(memory, memory_per_node, MPOL_BIND, &nodemask, sizeof(nodemask) * 8, 0) == -1) {
            numa_free(memory, memory_per_node);
            numa_bitmask_free(numa_nodes);
            throw std::runtime_error("Failed to set NUMA memory policy");
        }
    }

    numa_bitmask_free(numa_nodes);
}

// Explicit template instantiations for common types
template class AdaptiveCache<std::string, std::vector<uint8_t>>;
template class AdaptiveCache<uint64_t, std::string>;
template class AdaptiveCache<std::string, int>;
template class AdaptiveCache<int, std::string>;

} // namespace caching
} // namespace core 