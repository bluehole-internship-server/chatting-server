#pragma once
#include <vector>
#include <algorithm>
#include "Client.h"
#include "memory_pool.hpp"

namespace core
{
class ClientManager
{
friend class Server;
public:
	ClientManager();
	~ClientManager();
	Client * NewClient();
	VOID DeleteClient(Client *);
	VOID AddClient(Client *);
	VOID RemoveClient(Client *);

protected:
	std::vector<Client *>clients_;
	core::ObjectPool<Client> client_pool_;
	core::Spinlock lock_;
};
}

