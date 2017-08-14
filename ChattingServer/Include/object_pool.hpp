//
// object_pool.hpp
//

#pragma once

#include <unordered_map>
#include <unordered_set>

#include "Spinlock.h"

#include "pool.hpp"
#include "null_mutex.hpp"

namespace core {

template<typename T,
    unsigned RequestedSize = sizeof(T)>
class ObjectPool : public Pool {
public:
                                // 2bytes for HACK
    explicit ObjectPool() : Pool(RequestedSize + 2) { }
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
        T* ret = static_cast<T*>(object_pool()->Malloc());

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
    static const int max_thread_num = 64;

    inline ObjectPool<T, RequestedSize>* object_pool()
    {
        static long thread_id_counter = 0;

        if (object_pool_ == nullptr) {
            ref_count_++;
            object_pool_ = reinterpret_cast<ObjectPool<T, RequestedSize>*>
                (_aligned_malloc(sizeof(ObjectPool<T, RequestedSize>), 64));
            new (object_pool_)ObjectPool<T, RequestedSize>();
            thread_id_ = InterlockedAdd(&thread_id_counter, 1);
            object_pool_array_[thread_id_] = object_pool_;
        }

        return object_pool_;
    }

    static __declspec(thread) __declspec(align(64))
        ObjectPool<T, RequestedSize>* object_pool_;

    static __declspec(thread) __declspec(align(64))
        int ref_count_;

    static __declspec(thread) __declspec(align(64))
        short thread_id_;


    inline void* malloc()
    {
        char* ret = (char*)Pool::Malloc();
        *(short*)(ret + RequestedSize) = thread_id_;
        return ret;
    }

    inline void free(void* const chunk)
    {
        short thread_id = *(short*)((char*)chunk + RequestedSize);
        if (thread_id == thread_id_) {
            Pool::Free(chunk);
        }
        else {
            object_pool_array_[thread_id]->free_append(chunk);
        }
    }

    inline void free_append(void* const chunk)
    {
        Pool::FreeAppend(chunk);
    }

    inline bool purge_memory()
    {
        return Pool::PurgeMemory();
    }

    static ObjectPool<T, RequestedSize>* object_pool_array_[max_thread_num + 1];

};

template<typename T, unsigned RequestedSize>
typename ObjectPool<T, RequestedSize>*
ObjectPool<T, RequestedSize>::object_pool_ = nullptr;;

template<typename T, unsigned RequestedSize>
int ObjectPool<T, RequestedSize>::ref_count_ = 0;

template<typename T, unsigned RequestedSize>
typename ObjectPool<T, RequestedSize>*
    ObjectPool<T, RequestedSize>::object_pool_array_[ObjectPool<T,
    RequestedSize>::max_thread_num + 1];

template<typename T, unsigned RequestedSize>
short ObjectPool<T, RequestedSize>::thread_id_;

}