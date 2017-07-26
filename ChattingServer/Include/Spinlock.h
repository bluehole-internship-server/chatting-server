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

class SpinlockGuard
{
public:
	SpinlockGuard(Spinlock &spinlock);
	~SpinlockGuard();

private:
	Spinlock &spinlock_;
};
}
