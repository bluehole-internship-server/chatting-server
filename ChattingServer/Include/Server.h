#ifndef UNICODE
#define UNICODE
#endif

#pragma once
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"mswsock.lib")

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <stdio.h>
#include <functional>
#include "ThreadPool.h"
#include "ClientManager.h"
#include "memory_pool.hpp"

#define WORKER_AMOUNT 16
#define DEFAULT_PACKET_HEADER_SIZE 2

namespace core
{
class Server
{
public:
	Server();
	~Server();
	VOID Init();
	VOID SetListenPort(USHORT);
	template <class F, class... Args>
	auto AddWork(F&& f, Args&&... args)
		-> std::future<typename std::result_of<F(Args...)>::type>;
	static VOID IocpWork(Server &server);
	VOID Run();

	VOID SetPacketHeaderSize(USHORT size);
	VOID SetAcceptHandler(std::function<void(IoContext *)>);
	VOID SetReceiveHandler(std::function<void(IoContext *)>);
	VOID SetSendHandler(std::function<void(IoContext *)>);
	VOID SetDisconnectHandler(std::function<void(IoContext *)>);
	VOID AcceptHandler(IoContext *);
	VOID ReceiveHandler(IoContext *);
	VOID SendHandler(IoContext *);
	VOID DisconnectHandler(IoContext *);
	std::vector<Client *>& GetAllClient();

	static USHORT packet_header_size_;
	static LPFN_DISCONNECTEX DisconnectEx;
	static LPFN_ACCEPTEX AcceptEx;
	static LPFN_CONNECTEX ConnectEx;
	static char accept_buffer_[64];

private:
	static ThreadPool *thread_pool_;
	ClientManager * client_manager_;
	HANDLE completion_port_;
	SOCKET listen_socket_;
	USHORT listen_port_;
    ObjectPool<IoContext> io_context_pool_;
	std::function<void(IoContext *)> accept_handler_;
	std::function<void(IoContext *)> receive_handler_;
	std::function<void(IoContext *)> send_handler_;
	std::function<void(IoContext *)> disconnect_handler_;

	void PrintError(wchar_t * target, DWORD error_code);
};	
}
#include "Server.inl"