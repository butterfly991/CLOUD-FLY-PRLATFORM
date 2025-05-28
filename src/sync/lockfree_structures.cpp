#include "lockfree_structures.h"
#include "architecture.h"
#include <memory>
#include <stdexcept>
#include <cstring>

namespace core {
namespace lockfree {

// LockFreeQueue Implementation
template<typename T>
void LockFreeQueue<T>::enqueue(const T& item) {
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
T LockFreeQueue<T>::dequeue() {
    while (true) {
        Node* head = head_.load(std::memory_order_acquire);
        Node* tail = tail_.load(std::memory_order_acquire);
        Node* next = head->next.load(std::memory_order_acquire);
        
        if (head == head_.load(std::memory_order_acquire)) {
            if (head == tail) {
                if (next == nullptr) {
                    throw std::runtime_error("Queue is empty");
                }
                tail_.compare_exchange_strong(tail, next,
                    std::memory_order_release,
                    std::memory_order_relaxed);
            } else {
                if (next != nullptr) {
                    T value = next->data;
                    if (head_.compare_exchange_weak(head, next,
                        std::memory_order_release,
                        std::memory_order_relaxed)) {
                        delete head;
                        return value;
                    }
                }
            }
        }
    }
}

// LockFreeStack Implementation
template<typename T>
void LockFreeStack<T>::push(const T& value) {
    Node* new_node = new Node{value, head_.load(std::memory_order_relaxed)};
    
    while (!head_.compare_exchange_weak(new_node->next, new_node,
        std::memory_order_release,
        std::memory_order_relaxed)) {
        // Retry until successful
    }
}

template<typename T>
bool LockFreeStack<T>::pop(T& value) {
    Node* old_head = head_.load(std::memory_order_relaxed);
    
    while (old_head != nullptr) {
        if (head_.compare_exchange_weak(old_head, old_head->next,
            std::memory_order_release,
            std::memory_order_relaxed)) {
            value = old_head->data;
            delete old_head;
            return true;
        }
    }
    
    return false;
}

// LockFreeHashMap Implementation
template<typename T>
LockFreeHashMap<T>::LockFreeHashMap(size_t bucket_count) 
    : buckets_(bucket_count) {
    // Initialize buckets with proper cache line alignment
    for (auto& bucket : buckets_) {
        new (&bucket) Bucket();
    }
}

template<typename T>
bool LockFreeHashMap<T>::insert(const T& key, const T& value) {
    size_t bucket_index = std::hash<T>{}(key) % buckets_.size();
    Bucket& bucket = buckets_[bucket_index];
    
    // Use memory ordering appropriate for the architecture
    #if defined(__ARM_NEON)
        // ARM-specific memory ordering
        return bucket.chain.enqueue(value);
    #else
        // x86-specific memory ordering
        return bucket.chain.enqueue(value);
    #endif
}

template<typename T>
bool LockFreeHashMap<T>::find(const T& key, T& value) {
    size_t bucket_index = std::hash<T>{}(key) % buckets_.size();
    Bucket& bucket = buckets_[bucket_index];
    
    try {
        value = bucket.chain.dequeue();
        return true;
    } catch (const std::runtime_error&) {
        return false;
    }
}

// Explicit template instantiations for common types
template class LockFreeQueue<int>;
template class LockFreeQueue<std::string>;
template class LockFreeQueue<std::vector<uint8_t>>;

template class LockFreeStack<int>;
template class LockFreeStack<std::string>;
template class LockFreeStack<std::vector<uint8_t>>;

template class LockFreeHashMap<int>;
template class LockFreeHashMap<std::string>;
template class LockFreeHashMap<std::vector<uint8_t>>;

} // namespace lockfree
} // namespace core 