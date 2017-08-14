//
// pool.hpp
//

#pragma once

#include <cstdlib>

#include <algorithm>

#include "segregated_storage.hpp"

namespace core {

class Pool : private segregated_storage {
public:
    explicit Pool(const std::size_t requested_size)
        : next_size_(64)
        , alloc_size_((std::max)(requested_size, sizeof(void*)))
        , segregated_storage((std::max)(requested_size, sizeof(void*)))
        , first_(nullptr)
    {
    }

    ~Pool()
    {
        PurgeMemory();
    }

    virtual void* Malloc() 
    {
        if (!Empty()) {
            return storage().malloc();
        }
        return resize_and_malloc();
    }

    bool ReleaseMemory()
    {
        // TODO
    }

    bool PurgeMemory()
    {
        while (first_ != nullptr) {
            void* next = next_of(first_);
            ::free(first_);
            first_ = next;
        }
        next_size_ = 32;

        storage().head_ = nullptr;
        return true;
    }

    virtual inline void Free(void* const chunk)
    {
        storage().free(chunk);
    }

    virtual inline void FreeAppend(void* const chunk)
    {
        storage().free_append(chunk);
    }

    inline bool Empty()
    {
        return storage().empty();
    }

private:
    segregated_storage& storage()
    {
        return *this;
    }

    std::size_t next_size_;
    std::size_t alloc_size_;

    void* resize_and_malloc()
    {
        std::size_t total_size = next_size_ * alloc_size_;

        char* ptr = (char*)::malloc(total_size +
            /* size of simple header */ sizeof(void*));
        
        if (ptr == 0) return 0;

        storage().add_block(ptr + sizeof(void*), total_size, alloc_size_);

        next_of(ptr) = first_;
        first_ = ptr;

        next_size_ <<= 1;

        return storage().malloc();
    }

    void* first_;
};
}