#pragma once
#define NICKNAME_MAX_LENGTH 80
#define NICKNAME_MIN_LENGTH 2
#define MESSAGE_MAX_LENGTH 256

enum PacketType : unsigned short
{
	LOGIN_REQ,
	LOGIN_ANS,
	CHAT_SEND,
	CHAT_RECV,
	JOIN_REQ,
	JOIN_ANS
};

struct PacketHeader
{
	unsigned short size_;
	PacketType type_;
};

struct LoginRequestPacket
{
	PacketHeader header_;
	char user_name_[NICKNAME_MAX_LENGTH];
};

enum LoginAnswer : unsigned short
{
	FAIL_DUPLICATE,
	FAIL_TOO_SHORT,
	FAIL_TOO_LONG,
	FAIL_UNKNOWN,
	SUCCESS
};

struct LoginAnswerPacket
{
	PacketHeader header_;
	LoginAnswer answer_;
};

struct ChatSendPacket
{
	PacketHeader header_;
	char message_[MESSAGE_MAX_LENGTH];
};

enum ChatType : unsigned short
{
	NORMAL,
	NOTICE,
	WHISPER
};

struct ChatReceivePacket
{
	PacketHeader header_;
	ChatType type_;
	unsigned short nickname_length_;
	// Nickname + Message
	char data_[NICKNAME_MAX_LENGTH + MESSAGE_MAX_LENGTH];
};