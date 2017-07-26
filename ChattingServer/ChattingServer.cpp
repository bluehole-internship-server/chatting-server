#include <Server.h>
#include "Packet.h"
#include "ChatClient.h"
#include <unordered_map>
#include <vector>

int main()
{
	std::unordered_map<core::Client *, ChatClient *> chat_clients;
	core::Server server;
	server.SetListenPort(55150);
	//server.SetAcceptHandler([&server, &chat_clients](core::IoContext * io_context) {
	//	ChatClient * chat_client = new ChatClient();
	//	SecureZeroMemory(chat_client->GetNickname(), NICKNAME_MAX_LENGTH);
	//	chat_client->client_ = io_context->client_;
	//	chat_clients.insert({ io_context->client_ ,chat_client });
	//});
	server.SetReceiveHandler([&server, &chat_clients](core::IoContext * io_context) {
		core::Client * reciever = io_context->client_;
		
		// Get Recieved Data
		reciever->recv_buffer_.SetHead(0);
		auto recieved = io_context->received_;
		char * buffer = new char[recieved];
		memcpy(buffer, reciever->recv_buffer_.Read(), recieved);

		// Parse the Packet
		PacketHeader * packet_header = (PacketHeader *)buffer;
		unsigned int packet_size = packet_header->size_;
		PacketType packet_type = packet_header->type_;

		switch (packet_type) {
		case LOGIN_REQ:
			LoginAnswerPacket login_answer_packet;
			{
				login_answer_packet.answer_ = FAIL_UNKNOWN;
				login_answer_packet.header_.size_ = sizeof(USHORT);
				login_answer_packet.header_.type_ = LOGIN_ANS;
			}
			if(packet_size > NICKNAME_MAX_LENGTH) {
				login_answer_packet.answer_ = FAIL_TOO_LONG;
			}
			else if (packet_size < NICKNAME_MIN_LENGTH) {
				login_answer_packet.answer_ = FAIL_TOO_SHORT;
			}
			else {
				LoginRequestPacket * login_request_packet = (LoginRequestPacket *)buffer;
				char * requested_name = new char[packet_size + 1];
				strncpy(requested_name, login_request_packet->user_name_, packet_size);
				requested_name[packet_size] = 0;
				auto clients = server.GetAllClient();
				bool is_duplicated = false;
				for (auto c : chat_clients) {
					printf("%s %s\n", c.second->GetNickname(), requested_name);
					if (strncmp(c.second->GetNickname(), requested_name, packet_size) == 0) {
						if (strlen(c.second->GetNickname()) == packet_size) {
							is_duplicated = true;
							break;
						}
					}
				}
				if (is_duplicated) {
					login_answer_packet.answer_ = FAIL_DUPLICATE;
				}
				else {
					login_answer_packet.answer_ = SUCCESS;

					ChatClient * chat_client = new ChatClient();
					SecureZeroMemory(chat_client->GetNickname(), NICKNAME_MAX_LENGTH);
					chat_client->client_ = reciever;					
					chat_client->SetNickname(requested_name);
					chat_clients.insert({ reciever ,chat_client });
				}
			}
			reciever->Send((char *)&login_answer_packet, sizeof(login_answer_packet));
			break;
		case CHAT_SEND:
			if (chat_clients.find(reciever) == chat_clients.end()) {
				break;
			}
			auto chat_client = chat_clients.find(reciever)->second;
			ChatSendPacket * chat_send_packet = (ChatSendPacket *)buffer;
			char * recieved_message = new char[strlen(chat_client->GetNickname()) + 1 + packet_size + 1];
			strcpy(recieved_message, chat_client->GetNickname());
			recieved_message[strlen(chat_client->GetNickname())] = ':';
			strcpy(recieved_message + strlen(chat_client->GetNickname()) + 1, chat_send_packet->message_);
			recieved_message[strlen(chat_client->GetNickname()) + 1 + packet_size] = 0;
			printf("recv : %s\n", recieved_message);
			for (auto client : chat_clients) {
				client.second->client_->Send(recieved_message, strlen(recieved_message));
			}
			break;
		}
/*

		auto clients = server.GetAllClient();
		for (auto c : clients) {
			c->Send(buffer, recieved);
		}*/
	});
	server.Init();
	server.Run();
	return 0;
}