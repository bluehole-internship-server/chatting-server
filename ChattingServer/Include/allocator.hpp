//
// allocator.hpp
// Like boost.pool, there are PoolAllocator and FastPoolAllocator
// no thread safety guaranteed
//

#pragma once

#include "pool.hpp"

namespace core {

template <class T>
class allocator_interface {
public:
    typedef std::size_t    size_type;
    typedef std::ptrdiff_t difference_type;
    typedef T*             pointer;
    typedef const T*       const_pointer;
    typedef T&             reference;
    typedef const T&       const_reference;
    typedef T              value_type;

    pointer address(reference value) const {
        return &value;
    }

    const_pointer address(const_reference value) const {
        return &value;
    }
   
    size_type max_size() const throw() {
        return (std::numeric_limits<size_t>::max)() / sizeof(T);
    }

    virtual pointer allocate(size_type n, const void* = 0) = 0;
    virtual void deallocate(pointer p, size_type n) = 0;


protected:
    allocator_interface() throw() {}

};

template <class T>
class PoolAllocator : public allocator_interface<T> {
    // TODO
};

template <class T>
class FastPoolAllocator : public allocator_interface<T> {
public:
    FastPoolAllocator() : pool_(sizeof(T)) {}
    
    template <class U> 
    FastPoolAllocator(const FastPoolAllocator<U> &allocator): pool_(sizeof(U)) {}

    FastPoolAllocator(const FastPoolAllocator<T> &allocator): pool_(sizeof(T)) {}
    template<class U>
    struct rebind {
        typedef FastPoolAllocator<U> other;
    };

    virtual pointer allocate(size_type n, const void* = 0)
    {
        assert(n == 1);
        if (n != 1) return nullptr;

        return (pointer)pool_.Malloc();
    }

    virtual void deallocate(pointer p, size_type n)
    {
        assert(n == 1);
        if (n != 1) return;

        pool_.Free(p);
    }

private:
    Pool pool_;
    // pool per one STL container instance
};
}

