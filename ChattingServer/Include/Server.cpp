#include "Server.h"

LPFN_DISCONNECTEX core::Server::DisconnectEx = nullptr;
LPFN_ACCEPTEX core::Server::AcceptEx = nullptr;
LPFN_CONNECTEX core::Server::ConnectEx = nullptr;
char core::Server::accept_buffer_[64] = { 0, };
core::ThreadPool * core::Server::thread_pool_ = nullptr;

namespace core 
{
Server::Server()
{
	client_manager_ = new ClientManager();
}
Server::~Server()
{
}
VOID Server::Init()
{
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
		thread_pool_ = new ThreadPool(WORKER_AMOUNT + 10);
		for (int i = 0; i < WORKER_AMOUNT; ++i)
			thread_pool_->Enqueue(IocpWork, *this);
	}
}
VOID Server::SetListenPort(USHORT port)
{
	listen_port_ = port;
}
VOID Server::IocpWork(Server &server)
{
	while (TRUE) {
		DWORD received_bytes = 0;
		IoContext * io_context = nullptr;
		ULONG_PTR key = 0;
		BOOL io_result = false;
		GetQueuedCompletionStatus(server.completion_port_, &received_bytes, (PULONG_PTR)&key, (LPOVERLAPPED *)&io_context, INFINITE);
		io_context->received_ = received_bytes;
		auto client = io_context->client_;
		switch (io_context->io_type_) {
			case IO_ACCEPT:
				server.client_manager_->AddClient(client);
				io_result = client->PrepareReceive();
				server.AcceptHandler(io_context);
				break;
			case IO_RECV_READY:
				io_result = client->Receive();
				break;
			case IO_RECV:
				client->PostReceive(received_bytes, server.receive_handler_, *io_context);
				io_result = client->PrepareReceive();
				break;
			case IO_SEND:
				io_result = client->PostSend(received_bytes);
				server.SendHandler(io_context);
				break;
			case IO_DISCONNECT:
				server.client_manager_->RemoveClient(client);
				io_result = true;
				server.DisconnectHandler(io_context);
				break;
			default:
				wprintf(L"What? %d\n", io_context->io_type_);
				break;
		}

		if (io_result == FALSE)
		{
			client->Disconnect();
		}
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
        
		new_client->socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		IoContext * io_context = io_context_pool_.Construct(IoContext(new_client, IO_ACCEPT));
		SOCKET client_socket = io_context->client_->socket_;
		
		auto result = CreateIoCompletionPort((HANDLE)client_socket, completion_port_, (ULONG_PTR)client_socket, 0);
		_ASSERT(result == completion_port_);
				
		AcceptEx(listen_socket_, client_socket, accept_buffer_, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &received_bytes, (LPOVERLAPPED)io_context);
		Sleep(1000);
	}
}
VOID Server::SetAcceptHandler(std::function<void(IoContext *)> handler)
{
	accept_handler_ = std::move(handler);
}
VOID Server::SetReceiveHandler(std::function<void(IoContext *)> handler)
{
	receive_handler_ = std::move(handler);
}
VOID Server::SetSendHandler(std::function<void(IoContext *)> handler)
{
	send_handler_ = std::move(handler);
}
VOID Server::SetDisconnectHandler(std::function<void(IoContext *)> handler)
{
	disconnect_handler_ = std::move(handler);
}
VOID Server::AcceptHandler(IoContext * io_context)
{
	if (accept_handler_ != nullptr)
		accept_handler_(io_context);
}
VOID Server::ReceiveHandler(IoContext * io_context)
{
	if (receive_handler_ != nullptr)
		receive_handler_(io_context);
}
VOID Server::SendHandler(IoContext * io_context)
{
	if (send_handler_ != nullptr)
		send_handler_(io_context);
}
VOID Server::DisconnectHandler(IoContext * io_context)
{
	if (disconnect_handler_ != nullptr)
		disconnect_handler_(io_context);
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