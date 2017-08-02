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
	VOID SetFastSocketOption(Client *);
	template <class F, class... Args>
	auto AddWork(F&& f, Args&&... args)
		-> std::future<typename std::result_of<F(Args...)>::type>;
	static VOID IocpWork(Server &server);
	VOID Run();

	VOID SetPacketHeaderSize(USHORT size);
	VOID SetPreAcceptHandler(std::function<void(IoContext *)>);
	VOID SetPreReceiveHandler(std::function<void(IoContext *)>);
	VOID SetPreSendHandler(std::function<void(IoContext *)>);
	VOID SetPreDisconnectHandler(std::function<void(IoContext *)>);
	VOID SetPostAcceptHandler(std::function<void(IoContext *)>);
	VOID SetPostReceiveHandler(std::function<void(IoContext *)>);
	VOID SetPostSendHandler(std::function<void(IoContext *)>);
	VOID SetPostDisconnectHandler(std::function<void(IoContext *)>);
	VOID PreAcceptHandler(IoContext *);
	VOID PreReceiveHandler(IoContext *);
	VOID PreSendHandler(IoContext *);
	VOID PreDisconnectHandler(IoContext *);
	VOID PostAcceptHandler(IoContext *);
	VOID PostReceiveHandler(IoContext *);
	VOID PostSendHandler(IoContext *);
	VOID PostDisconnectHandler(IoContext *);
	std::vector<Client *>& GetAllClient();

	static USHORT packet_header_size_;
	static LPFN_DISCONNECTEX DisconnectEx;
	static LPFN_ACCEPTEX AcceptEx;
	static LPFN_CONNECTEX ConnectEx;
	static char accept_buffer_[64];
	static ObjectPool<IoContext> * io_context_pool_;

private:
	static ThreadPool *thread_pool_;
	ClientManager * client_manager_;
	HANDLE completion_port_;
	SOCKET listen_socket_;
	USHORT listen_port_;
	std::function<void(IoContext *)> pre_accept_handler_;
	std::function<void(IoContext *)> pre_receive_handler_;
	std::function<void(IoContext *)> pre_send_handler_;
	std::function<void(IoContext *)> pre_disconnect_handler_;
	std::function<void(IoContext *)> post_accept_handler_;
	std::function<void(IoContext *)> post_receive_handler_;
	std::function<void(IoContext *)> post_send_handler_;
	std::function<void(IoContext *)> post_disconnect_handler_;

	void PrintError(wchar_t * target, DWORD error_code);
};	
}
#include "Server.inl"