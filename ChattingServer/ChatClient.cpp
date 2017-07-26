#include "ChatClient.h"

char * ChatClient::GetNickname()
{
	return nickname_;
}

void ChatClient::SetNickname(char * nickname)
{
	strcpy(nickname_, nickname);
}
