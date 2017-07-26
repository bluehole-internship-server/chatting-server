#pragma once
#include <Client.h>

class ChatClient : public core::Client
{
public:
	ChatClient();
	~ChatClient();

private:
	char nickname[80];
};

