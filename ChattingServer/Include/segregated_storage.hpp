//
// segregated_storage.hpp
//

#pragma once

#include <cstddef>

namespace core {

class Pool;

class segregated_storage {
public:
    segregated_storage(std::size_t chunk_size)
        : first_(nullptr)
    {
        first_ = nullptr;
    }

    void* malloc()
    {
        void* const ret = first_;
        first_ = next_of(first_);
        return ret;
    }

    void free(void* const chunk)
    {
        next_of(chunk) = first_;
        first_ = chunk;
    }
    
    inline bool empty()
    {
        return (first_ == nullptr);
    }

    inline void add_block(void* const block, const std::size_t sz,
        const std::size_t partition_sz)
    {
        first_ = segregate(block, sz, partition_sz, first_);
    }

    static void* segregate(void* block, std::size_t sz,
        std::size_t partition_sz, void* end)
    {
        char* last = static_cast<char*>(block) + sz - partition_sz;
        next_of(last) = end;

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

    void* first_;
};
}