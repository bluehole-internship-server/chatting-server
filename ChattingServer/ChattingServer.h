#pragma once
#include <Server.h>
#include "Packet.h"
#include "ChatClient.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <chrono>
#include <algorithm>
#include <Spinlock.h>


struct ReturnPacketCounter
{
	ChatReceivePacket * return_packet_;
	unsigned int target_;
};
enum class Command : unsigned short
{
	NONE, UP, DOWN, LEFT, RIGHT
};


std::unordered_map<unsigned int, ReturnPacketCounter *> return_packet_counters;
std::unordered_map<core::Client *, ChatClient *> chat_clients;
std::unordered_set<std::string> client_names;
std::unordered_map<std::string, unsigned int> game_commands;
std::unordered_map<std::string, const unsigned char> game_command_index = { { "¸ØÃç", 0 },{ "À§", 1 },{ "¾Æ·¡", 2 },{ "¿ÞÂÊ", 3 },{ "¿À¸¥ÂÊ", 4 } };

LoginAnswerPacket * CreateLoginAnswerPacket(unsigned int packet_size);
bool FindDuplicateNickname(
	std::unordered_set<std::string> &client_names,
	char * requested_name);
ChatReceivePacket * CreateChatReturnPacket(
	unsigned short send_message_length,
	char * nickname,
	unsigned int nickname_length,
	char * message,
	unsigned int message_length);
void SetGameCommands(std::unordered_map<std::string, unsigned int> &game_commands);
ChatType ProcessCommand(char * msg, ChatClient * chat_client, unsigned short packet_size, unsigned short chat_client_nickname_length);
void ReturnBroadcast(ChatClient * chat_client, unsigned short chat_client_nickname_length, ChatSendPacket * chat_send_packet, unsigned short send_message_length, ChatType chat_type, unsigned short packet_size);