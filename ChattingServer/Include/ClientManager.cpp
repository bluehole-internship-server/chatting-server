#include "ClientManager.h"

namespace core
{
ClientManager::ClientManager()
{
}
ClientManager::~ClientManager()
{
}
Client * ClientManager::NewClient()
{
	Client * new_client = reinterpret_cast<Client*>(client_pool_.Malloc());
	new (new_client) Client();
	return new_client;
}
VOID ClientManager::DeleteClient(Client * client)
{
	client_pool_.Destroy(client);
}
VOID ClientManager::AddClient(Client * client)
{
	SpinlockGuard lock(lock_);
	clients_.push_back(client);
}
VOID ClientManager::RemoveClient(Client * client)
{
	SpinlockGuard lock(lock_);
	auto it = std::find(clients_.begin(), clients_.end(), client);
	if (it != clients_.end()) {
		DeleteClient(*it);
		clients_.erase(it);
	}
}
}