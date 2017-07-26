//
// null_mutex.hpp
//

#pragma once

namespace core {

class null_mutex {
public:
    inline void lock() {}
    inline void unlock() {}
    inline bool try_lock() { return true; }
};
}
