//
// object_pool.hpp
//

#pragma once

#include "pool.hpp"
#include "null_mutex.hpp"

namespace core {

template<typename T,
    unsigned RequestedSize = sizeof(T)>
class ObjectPool : public Pool {
public:
    explicit ObjectPool() : Pool(RequestedSize) { }
    ~ObjectPool()
    {
        if (object_pool_ == nullptr) return;

        ref_count_--;
        if (ref_count_ == 0) {
            _aligned_free(object_pool_);
        }
    }

    bool Preallocate(int n)
    {
        bool ret = true;
        void** arr = reinterpret_cast<void**>(::malloc(sizeof(void*) * n));
        for (int i = 0; i < n; i++) arr[i] = object_pool()->malloc();
        for (int i = 0; i < n; i++) {
            ret = ret & (arr[i] != nullptr);
            if (arr[i] != nullptr) object_pool()->free(arr[i]);
        }

        return ret;
    }

    inline virtual void* Malloc() override
    {
        return object_pool()->malloc();
    }

    inline virtual void Free(void* const chunk) override
    {
        return object_pool()->free(chunk);
    }

    inline T* Construct(T&& t)
    {
        T* ret =
            static_cast<T*>(object_pool()->Malloc());

        try {
            new (ret) T(std::move(t));
        }
        catch (...) {
            object_pool()->Free(ret);
            ret = nullptr;
        }

        return ret;
    }

    inline void Destroy(T* const chunk)
    {
        chunk->~T();
        object_pool()->Free(chunk);
    }

    inline bool PurgeMemory()
    {
        return object_pool()->purge_memory();
    }

private:
    inline ObjectPool<T, RequestedSize>* object_pool()
    {
        if (object_pool_ == nullptr) {
            ref_count_++;
            object_pool_ = reinterpret_cast<ObjectPool<T, RequestedSize>*>
                (_aligned_malloc(sizeof(ObjectPool<T, RequestedSize>), 64));
            new (object_pool_)ObjectPool<T, RequestedSize>();
        }

        return object_pool_;
    }

    static __declspec(thread) __declspec(align(64))
        ObjectPool<T, RequestedSize>* object_pool_;

    static __declspec(thread) __declspec(align(64))
        int ref_count_;

    inline void* malloc()
    {
        return Pool::Malloc();
    }

    inline void free(void* const chunk)
    {
        Pool::Free(chunk);
    }

    inline bool purge_memory()
    {
        return Pool::PurgeMemory();
    }
};

template<typename T, unsigned RequestedSize>
typename ObjectPool<T, RequestedSize>*
ObjectPool<T, RequestedSize>::object_pool_ = nullptr;;

template<typename T, unsigned RequestedSize>
typename int ObjectPool<T, RequestedSize>::ref_count_ = 0;

}