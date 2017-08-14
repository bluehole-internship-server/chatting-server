//
// segregated_storage.hpp
//

#pragma once

#include <cstddef>
#include <cassert>

#include "Spinlock.h"

namespace core {

class Pool;

class segregated_storage {
public:
    segregated_storage(std::size_t chunk_size)
        : head_(nullptr)
        , tail_(nullptr)
    {
    }

    void* malloc()
    {
        void* const ret = head_;
        head_ = next_of(head_);
        return ret;
    }

    void free(void* const chunk)
    {
        next_of(chunk) = head_;
        head_ = chunk;
    }

    void free_append(void* const chunk)
    {
        assert(tail_ != nullptr);
        // Lock
        lock_tail_.lock();
        next_of(tail_) = chunk;
        tail_ = chunk;
        lock_tail_.unlock();
        // Unlock
    }
    
    inline bool empty()
    {
        return (head_ == tail_);
    }

    inline void add_block(void* const block, const std::size_t sz,
        const std::size_t partition_sz)
    {
        head_ = segregate(block, sz, partition_sz);
    }

    void* segregate(void* block, std::size_t sz,
        std::size_t partition_sz)
    {
        char* last = static_cast<char*>(block) + sz - partition_sz;
        if (tail_ == nullptr) {
            tail_ = last;
            next_of(tail_) = nullptr;
        }
        else {
            next_of(last) = tail_;
        }

        for (char* it = last - partition_sz;; it -= partition_sz) {
            next_of(it) = last;
            last = it;
            if (it == block) break;
        }

        return block;
        // TODO : How can i prevent overflow?
    }

    static inline void* & next_of(void* const ptr)
    {
        return *(static_cast<void **>(ptr));
    }

    void* head_;
    void* tail_;
    Spinlock lock_tail_;
};
}