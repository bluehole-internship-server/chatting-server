#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <future>
#include "Spinlock.h"

namespace core
{
class ThreadPool
{
public:
	ThreadPool(size_t thread_amount);
	~ThreadPool();
	template<class F, class... Args>
	auto inline Enqueue(F&& f, Args&&... args)
		->std::future<typename std::result_of<F(Args...)>::type>
	{
		using return_type = typename std::result_of<F(Args...)>::type;

		auto task = std::make_shared< std::packaged_task<return_type()> >(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
			);

		std::future<return_type> res = task->get_future();
		{
			SpinlockGuard lockGuard(spinlock_);
			if (stop)
				throw std::runtime_error("enqueue on stopped ThreadPool");

			tasks_.emplace([task]() { (*task)(); });
		}
		return res;
	}
private:
	std::queue<std::function<void()>> tasks_;
	std::vector<std::thread> workers_;
	Spinlock spinlock_;
	bool stop;
};
}
