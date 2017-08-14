#pragma once
#include <atomic>
#include <Windows.h>

namespace core 
{
class Spinlock
{
public:
	Spinlock();
    ~Spinlock();

	void Lock();
	void Unlock();
	void ReadLock();
	void ReadUnlock();

	void lock() { Lock(); }
	void unlock() { Unlock(); }

private:
	std::atomic<long> lock_;
	std::atomic<long> reader_;
};

class ExclusiveLockHolder
{
public:
	ExclusiveLockHolder(Spinlock &spinlock);
	~ExclusiveLockHolder();

private:
	Spinlock &spinlock_;
};

class SharedLockHolder
{
public:
	SharedLockHolder(Spinlock &spinlock);
	~SharedLockHolder();

private:
	Spinlock &spinlock_;
};
}
