#pragma once

enum PacketType : unsigned short
{
	LOGIN_REQ,
	LOGIN_ANS,
	CHAT_SEND,
	CHAT_RECV,
	JOIN_REQ,
	JOIN_ANS
};

typedef struct PacketHeader
{
	unsigned short size_;
};

typedef struct LoginRequestPacket
{
	char user_name_[80];
};

enum LoginAnswer : unsigned short
{
	SUCCESS,
	FAIL_DUPLICATE,
	FAIL_TOO_SHORT,
	FAIL_TOO_LONG,
	FAIL_UNKNOWN
};

typedef struct LoginAnswerPacket
{
	LoginAnswer answer_;
};

typedef struct ChatSendPacket
{
	char message_[256];
};

enum ChatType : unsigned short
{
	NORMAL,
	NOTICE,
	WHISPER
};

typedef struct ChatReceivePacket
{
	ChatType type_;
	char user_name_[80];
	char message_[256];
};