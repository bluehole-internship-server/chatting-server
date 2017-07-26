#pragma once
#include "ThreadPool.h"

namespace core
{	
	ThreadPool::ThreadPool(size_t thread_amount) : stop(false)
	{
		for (size_t i = 0; i < thread_amount; ++i) {
			workers_.emplace_back(
				[this]()
			{
				for (;;) {
					std::function<void()> task = nullptr;
					{
						SpinlockGuard lockGuard(spinlock_);
						if (stop && tasks_.empty()) {
							return;
						}
						else if (!tasks_.empty()) {
							task = std::move(tasks_.front());
							tasks_.pop();
						}
					}	
					if (task != nullptr)
						task();
				}
	
			});
		}
	}
	ThreadPool::~ThreadPool()
	{
		{
			SpinlockGuard lockGuard(spinlock_);
			stop = true;
		}
		for (auto &w : workers_)
			w.join();
	}
}