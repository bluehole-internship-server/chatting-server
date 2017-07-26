#pragma once
#include <Client.h>

class ChatClient
{
public:
	char * GetNickname();
	void SetNickname(char * nickname);
	core::Client * client_;
private:
	char nickname_[80];
};

