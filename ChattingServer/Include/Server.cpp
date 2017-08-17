#include "Server.h"

USHORT core::Server::packet_header_size_ = 2;
LPFN_DISCONNECTEX core::Server::DisconnectEx = nullptr;
LPFN_ACCEPTEX core::Server::AcceptEx = nullptr;
LPFN_CONNECTEX core::Server::ConnectEx = nullptr;
char core::Server::accept_buffer_[64] = { 0, };
core::ObjectPool<core::IoContext, 70> * core::Server::io_context_pool_ = nullptr;

namespace core 
{
Server::Server()
{
}
Server::~Server()
{
}
VOID Server::Init()
{
	client_manager_ = new ClientManager();
	SetPacketHeaderSize(DEFAULT_PACKET_HEADER_SIZE);

	int result = 0;

	// WinSock Init
	WSADATA wsa_data;
	result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	_ASSERT( result == NO_ERROR);

	// Listen
	listen_socket_ = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	_ASSERT(listen_socket_ != INVALID_SOCKET);
	int opt = 1;
	setsockopt(listen_socket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));

	// Bind on 0.0.0.0::listen_port_
	SOCKADDR_IN server_addr;
	SecureZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(listen_port_);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	result = bind(listen_socket_, (SOCKADDR *)&server_addr, sizeof(server_addr));
	_ASSERT( result != SOCKET_ERROR);

	// Get Function Pointers
	DWORD bytes = 0; 
	GUID guidDisconnectEx = WSAID_DISCONNECTEX;
	result = WSAIoctl(listen_socket_, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidDisconnectEx, sizeof(GUID), &DisconnectEx, sizeof(LPFN_DISCONNECTEX), &bytes, NULL, NULL);
	_ASSERT( result != SOCKET_ERROR);

	GUID guidAcceptEx = WSAID_ACCEPTEX;
	result = WSAIoctl(listen_socket_, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidAcceptEx, sizeof(GUID), &AcceptEx, sizeof(LPFN_ACCEPTEX), &bytes, NULL, NULL);
	_ASSERT( result != SOCKET_ERROR);

	GUID guidConnectEx = WSAID_CONNECTEX;
	result = WSAIoctl(listen_socket_, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidConnectEx, sizeof(GUID), &ConnectEx, sizeof(LPFN_CONNECTEX), &bytes, NULL, NULL);
	_ASSERT( result != SOCKET_ERROR);

	// Create and Associate IOCP
	completion_port_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	_ASSERT(completion_port_ != NULL);
	HANDLE associated_port = CreateIoCompletionPort((HANDLE)listen_socket_, completion_port_, (ULONG)listen_socket_, 0);
	_ASSERT(completion_port_ == associated_port);
	 
	// Create New Thread Pool
	if (thread_pool_ == nullptr) {
		thread_pool_ = new ThreadPool(worker_amount_);
		for (int i = 0; i < iocp_worker_amount_; ++i)
			thread_pool_->Enqueue(IocpWork, *this);
	}
	
	// Create New IoContext Pool
	if(io_context_pool_ == nullptr){
		io_context_pool_ = new core::ObjectPool<core::IoContext, 70>;
	}
}
VOID Server::SetListenPort(USHORT port)
{
	listen_port_ = port;
}
VOID Server::SetFastSocketOption(Client * client)
{
	int opt = 0;
	setsockopt(client->socket_, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int));
	LINGER lingerOption;
	lingerOption.l_onoff = 1;
	lingerOption.l_linger = 0;
	setsockopt(client->socket_, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof(LINGER));
}
VOID Server::IocpWork(Server &server)
{
	while (TRUE) {
		DWORD received_bytes = 0;
		IoContext * io_context = nullptr;
		ULONG_PTR key = 0;
		BOOL io_result = false;
		BOOL gqcs_result = GetQueuedCompletionStatus(server.completion_port_, &received_bytes, (PULONG_PTR)&key, (LPOVERLAPPED *)&io_context, 100);
		
		if (gqcs_result == 0 || received_bytes == 0) {
            if (io_context == nullptr && GetLastError() == WAIT_TIMEOUT) {
                server.TimeoutHandler(io_context);
				continue;
            }
            else if (io_context->io_type_ == IO_RECV || io_context->io_type_ == IO_SEND) {
                io_context->client_->Disconnect();
				io_context_pool_->Destroy(io_context);
				continue;
            }
        }

		io_context->received_ = received_bytes;
		auto client = io_context->client_;
		switch (io_context->io_type_) {
			case IO_ACCEPT:
				server.PreAcceptHandler(io_context);
				server.SetFastSocketOption(client);
				server.client_manager_->AddClient(client);
				io_result = client->PrepareReceive();
				server.PostAcceptHandler(io_context);
				break;
			case IO_RECV_READY:
				io_result = client->Receive();
				break;
			case IO_RECV:
				client->PostReceive(received_bytes, server.post_receive_handler_, *io_context);
				io_result = client->PrepareReceive();
				break;
			case IO_SEND:
				server.PreSendHandler(io_context);
				io_result = client->PostSend(received_bytes);
				server.PostSendHandler(io_context);
				break;
			case IO_DISCONNECT:
				server.PreDisconnectHandler(io_context);
				server.client_manager_->RemoveClient(client);
				io_result = true;
				server.PostDisconnectHandler(io_context);
				break;
			default:
				throw "Unknown Io Type";
				break;
		}

		if (io_result == FALSE)
		{
			client->Disconnect();
		}
		io_context_pool_->Destroy(io_context);
	}
}
VOID Server::Run()
{
	int result = 0;
	result = listen(listen_socket_, SOMAXCONN);
	_ASSERT( result != SOCKET_ERROR);
	
	while (TRUE) {
		DWORD received_bytes;

		Client * new_client = client_manager_->NewClient();
		if (new_client != nullptr) {
			new_client->socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			IoContext * io_context = io_context_pool_->Construct(IoContext(new_client, IO_ACCEPT));
			SOCKET client_socket = io_context->client_->socket_;

			auto result = CreateIoCompletionPort((HANDLE)client_socket, completion_port_, (ULONG_PTR)client_socket, 0);
			_ASSERT(result == completion_port_);

			AcceptEx(listen_socket_, client_socket, accept_buffer_, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &received_bytes, (LPOVERLAPPED)io_context);
		}
		Sleep(1);
	}
}
VOID Server::SetPacketHeaderSize(USHORT size)
{
	packet_header_size_ = size;
}
VOID Server::SetWorkerAmount(USHORT amount)
{
	worker_amount_ = amount;
}
VOID Server::SetIocpWorkerAmount(USHORT amount)
{
	iocp_worker_amount_ = amount;
}
VOID Server::SetTimeoutHandler(std::function<void(IoContext*)> handler)
{
	timeout_handler_ = std::move(handler);
}
VOID Server::SetPreAcceptHandler(std::function<void(IoContext *)> handler)
{
	pre_accept_handler_ = std::move(handler);
}
VOID Server::SetPreReceiveHandler(std::function<void(IoContext *)> handler)
{
	pre_receive_handler_ = std::move(handler);
}
VOID Server::SetPreSendHandler(std::function<void(IoContext *)> handler)
{
	pre_send_handler_ = std::move(handler);
}
VOID Server::SetPreDisconnectHandler(std::function<void(IoContext *)> handler)
{
	pre_disconnect_handler_ = std::move(handler);
}
VOID Server::SetPostAcceptHandler(std::function<void(IoContext *)> handler)
{
	post_accept_handler_ = std::move(handler);
}
VOID Server::SetPostReceiveHandler(std::function<void(IoContext *)> handler)
{
	post_receive_handler_ = std::move(handler);
}
VOID Server::SetPostSendHandler(std::function<void(IoContext *)> handler)
{
	post_send_handler_ = std::move(handler);
}
VOID Server::SetPostDisconnectHandler(std::function<void(IoContext *)> handler)
{
	post_disconnect_handler_ = std::move(handler);
}
VOID Server::TimeoutHandler(IoContext * io_context)
{
	if (timeout_handler_ != nullptr)
		timeout_handler_(io_context);
}
VOID Server::PreAcceptHandler(IoContext * io_context)
{
	if (pre_accept_handler_ != nullptr)
		pre_accept_handler_(io_context);
}
VOID Server::PreReceiveHandler(IoContext * io_context)
{
	if (pre_receive_handler_ != nullptr)
		pre_receive_handler_(io_context);
}
VOID Server::PreSendHandler(IoContext * io_context)
{
	if (pre_send_handler_ != nullptr)
		pre_send_handler_(io_context);
}
VOID Server::PreDisconnectHandler(IoContext * io_context)
{
	if (pre_disconnect_handler_ != nullptr)
		pre_disconnect_handler_(io_context);
}
VOID Server::PostAcceptHandler(IoContext * io_context)
{
	if (post_accept_handler_ != nullptr)
		post_accept_handler_(io_context);
}
VOID Server::PostReceiveHandler(IoContext * io_context)
{
	if (post_receive_handler_ != nullptr)
		post_receive_handler_(io_context);
}
VOID Server::PostSendHandler(IoContext * io_context)
{
	if (post_send_handler_ != nullptr)
		post_send_handler_(io_context);
}
VOID Server::PostDisconnectHandler(IoContext * io_context)
{
	if (post_disconnect_handler_ != nullptr)
		post_disconnect_handler_(io_context);
}
std::vector<Client*>& Server::GetAllClient()
{
	return client_manager_->clients_;
}
void Server::PrintError(wchar_t * target, DWORD error_code)
{
	wprintf(L"%s가 실패했습니다. 에러 코드: %d\n", target, error_code);
}
}