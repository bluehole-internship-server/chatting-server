#include "Client.h"
#include "Server.h"

namespace core
{
Client::Client()
{
	send_buffer_.Init(SEND_BUFFER_SIZE);
	recv_buffer_.Init(RECV_BUFFER_SIZE);
}
Client::~Client()
{
	puts("Good Bye.");
}
BOOL Client::PrepareReceive()
{
	IoContext * recv_ready_context = new IoContext(this, IO_RECV_READY);
	DWORD recieved_bytes = 0;
	DWORD flags = 0;

	WSARecv(socket_, &recv_ready_context->buffer_, 1, &recieved_bytes, &flags, (LPWSAOVERLAPPED)recv_ready_context, NULL);
	return TRUE;
}
BOOL Client::Receive()
{
	wprintf(L"Receive is Prepared.\n");

	IoContext * recv_context = new IoContext(this, IO_RECV);
	DWORD recieved_bytes = 0;
	DWORD flags = 0;
	recv_context->buffer_.len = recv_buffer_.GetRemained();
	recv_context->buffer_.buf = recv_buffer_.GetBuffer();

	if (WSARecv(socket_, &recv_context->buffer_, 1, &recieved_bytes, &flags, (LPWSAOVERLAPPED)recv_context, NULL) == SOCKET_ERROR)
		return FALSE;
	return TRUE;
}
BOOL Client::PostReceive(DWORD received_bytes, std::function<void(IoContext *)>& handler, IoContext& io_context)
{
	wprintf(L"Received.\n");
	UWORD packet_size = ((UWORD)*(recv_buffer_.Read()));
	if(received_bytes >= packet_size - recv_buffer_.offset_)
	{
		recv_buffer_.SetHead(sizeof(UWORD));
		recv_buffer_.Produce(packet_size + sizeof(UWORD));
		*(recv_buffer_.GetBuffer()) = 0;
		printf("Received Data : %s\n", recv_buffer_.Read());
		handler(&io_context);
		recv_buffer_.Consume(packet_size + sizeof(UWORD));
	}

	return TRUE;
}
BOOL Client::Send(char * data, DWORD size)
{
	IoContext * send_context = new IoContext(this, IO_SEND);
	DWORD sent, flags;

	memcpy(send_buffer_.GetBuffer(), data, size);
	send_context->buffer_.len = size;
	send_context->buffer_.buf = send_buffer_.GetBuffer();
	send_buffer_.Produce(size);
		
	WSASend(socket_, &(send_context->buffer_), 1, &sent, 0, (LPWSAOVERLAPPED)send_context, nullptr);
	return TRUE;
}
BOOL Client::PostSend(DWORD sent_bytes)
{
	send_buffer_.Consume(sent_bytes);
	return TRUE;
}
BOOL Client::Disconnect()
{
	IoContext * disconnect_context = new IoContext(this, IO_DISCONNECT);
	core::Server::DisconnectEx(socket_, (LPWSAOVERLAPPED)disconnect_context, 0, 0);
	return 0;
}
}