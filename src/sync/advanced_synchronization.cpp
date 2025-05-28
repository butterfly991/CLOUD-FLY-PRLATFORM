#include "advanced_synchronization.h"
#include "architecture.h"
#include <thread>
#include <stdexcept>
#include <chrono>

namespace core {
namespace concurrency {

// HierarchicalLock Implementation
thread_local uint64_t HierarchicalLock::current_hierarchy = 0;

void HierarchicalLock::lock(uint64_t hierarchy) {
    if (hierarchy <= current_hierarchy) {
        throw std::runtime_error("Invalid lock hierarchy");
    }
    
    uint64_t expected = 0;
    while (!lock_.compare_exchange_weak(expected, hierarchy,
        std::memory_order_acquire,
        std::memory_order_relaxed)) {
        expected = 0;
        
        // Architecture-specific spin wait
        #if defined(__ARM_NEON)
            // ARM-specific spin wait using WFE instruction
            __asm__ __volatile__("wfe");
        #else
            // x86-specific spin wait using PAUSE instruction
            _mm_pause();
        #endif
    }
    
    current_hierarchy = hierarchy;
}

void HierarchicalLock::unlock() {
    if (current_hierarchy == 0) {
        throw std::runtime_error("Attempting to unlock when not locked");
    }
    
    lock_.store(0, std::memory_order_release);
    current_hierarchy = 0;
}

// RCUGuard Implementation
thread_local uint64_t RCUGuard::version_ = 0;

RCUGuard::RCUGuard() {
    version_ = version_.load(std::memory_order_acquire);
}

RCUGuard::~RCUGuard() {
    // No cleanup needed
}

void RCUGuard::synchronize() {
    // Wait for all readers to complete
    uint64_t current_version = version_.load(std::memory_order_acquire);
    
    // Architecture-specific synchronization
    #if defined(__ARM_NEON)
        // ARM-specific memory barrier
        __asm__ __volatile__("dmb ish");
    #else
        // x86-specific memory barrier
        _mm_mfence();
    #endif
    
    // Wait for all readers to complete
    while (version_.load(std::memory_order_acquire) == current_version) {
        std::this_thread::yield();
    }
}

// LockFreeQueue Implementation
template<typename T>
void LockFreeQueue<T>::enqueue(T item) {
    Node* new_node = new Node{item, nullptr};
    
    while (true) {
        Node* tail = tail_.load(std::memory_order_acquire);
        Node* next = tail->next.load(std::memory_order_acquire);
        
        if (tail == tail_.load(std::memory_order_acquire)) {
            if (next == nullptr) {
                if (tail->next.compare_exchange_weak(next, new_node,
                    std::memory_order_release,
                    std::memory_order_relaxed)) {
                    tail_.compare_exchange_strong(tail, new_node,
                        std::memory_order_release,
                        std::memory_order_relaxed);
                    return;
                }
            } else {
                tail_.compare_exchange_strong(tail, next,
                    std::memory_order_release,
                    std::memory_order_relaxed);
            }
        }
    }
}

template<typename T>
bool LockFreeQueue<T>::dequeue(T& result) {
    while (true) {
        Node* head = head_.load(std::memory_order_acquire);
        Node* tail = tail_.load(std::memory_order_acquire);
        Node* next = head->next.load(std::memory_order_acquire);
        
        if (head == head_.load(std::memory_order_acquire)) {
            if (head == tail) {
                if (next == nullptr) {
                    return false;
                }
                tail_.compare_exchange_strong(tail, next,
                    std::memory_order_release,
                    std::memory_order_relaxed);
            } else {
                if (next != nullptr) {
                    result = next->data;
                    if (head_.compare_exchange_weak(head, next,
                        std::memory_order_release,
                        std::memory_order_relaxed)) {
                        delete head;
                        return true;
                    }
                }
            }
        }
    }
}

// Explicit template instantiations for common types
template class LockFreeQueue<int>;
template class LockFreeQueue<std::string>;
template class LockFreeQueue<std::vector<uint8_t>>;

} // namespace concurrency
} // namespace core 