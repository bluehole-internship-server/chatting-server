#pragma once
#include <WinSock2.h>
#include <stdio.h>
#include "IoContext.h"
#include "Buffer.h"
#include <functional>

#define SEND_BUFFER_SIZE 512
#define RECV_BUFFER_SIZE 512
namespace core
{
class Client
{
public:
	Client();
	~Client();
	BOOL PrepareReceive();
	BOOL Receive();
	BOOL PostReceive(DWORD, std::function<void(IoContext *)>&, IoContext&);
	BOOL Send(char *, DWORD);
	BOOL PostSend(DWORD);
	BOOL Disconnect();

	SOCKET socket_;
	Buffer send_buffer_;
	Buffer recv_buffer_;
};
}