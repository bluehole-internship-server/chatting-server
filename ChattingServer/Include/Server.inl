#include "Server.h"

namespace core
{
	template <class F, class... Args>
	auto Server::AddWork(F&& f, Args&&... args)
		-> std::future<typename std::result_of<F(Args...)>::type> {
		return thread_pool_.Enqueue(f, args...);
	}
}