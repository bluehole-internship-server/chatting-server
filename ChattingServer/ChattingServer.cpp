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
	server.SetDisconnectHandler([&server, &chat_clients](core::IoContext * io_context) {
		auto target = chat_clients.find(io_context->client_);
		if (target == chat_clients.end()) {
			// Do Something.
		}
		else {
			//ChatClient * disconnected_client = target->second;
			printf("User \'%s\' Leave.\n", target->second->GetNickname());
			delete target->second;
			chat_clients.erase(io_context->client_);
		}
	});
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
				memcpy(requested_name, login_request_packet->user_name_, packet_size);
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
			auto target = chat_clients.find(reciever);
			if (target == chat_clients.end()) {
				// Do Something.
			}
			else {
				ChatClient * chat_client = target->second;
				ChatSendPacket * chat_send_packet = (ChatSendPacket *)buffer;
				unsigned short chat_client_nickname_length = (unsigned short)strlen(chat_client->GetNickname());

				// Tokenize
				if (packet_size != 0) {
					auto msg = chat_send_packet->message_;
					bool is_command_able = (msg[0] == '/' || msg[0] == '!');
					if (is_command_able) {
						char command_type = msg[0];
						char * command_body = nullptr;
						unsigned short command_length = 0;
						switch (command_type) {
						case '/':
							if (packet_size >= 4) {
								char command = msg[1];
								printf("Received Chatting Command is \'%c\'\n", command);
								command_body = msg + 3;
								command_length = packet_size - 3;
								switch (command) {
								case 'w':
									break;
								default:
									break;
								}
							}
							break;
						case '!':
							command_body = msg + 1;
							command_length = packet_size - 1;
							printf("Received Game Command is ");
							for (int i = 0; i < command_length; ++i) {
								putc(command_body[i], stdout);
							}	putc('\n', stdout);
							break;
						default:
							break;
						}

					}
					else {
						unsigned short send_message_length = sizeof(ChatReceivePacket::type_) + sizeof(ChatReceivePacket::nickname_length_) + chat_client_nickname_length + packet_size;
						if (send_message_length > MESSAGE_MAX_LENGTH) {
							// Do Something.
						}
						else {
							ChatReceivePacket * chat_receive_packet = new ChatReceivePacket();
							chat_receive_packet->header_.type_ = CHAT_RECV;

							char * return_data = chat_receive_packet->data_;
							// Nickname is **NOT** Null Terminated.
							memcpy(return_data, chat_client->GetNickname(), chat_client_nickname_length);
							memcpy(return_data + chat_client_nickname_length, chat_send_packet->message_, packet_size);
							return_data[send_message_length - 1] = 0;
							printf("%s\n", return_data);

							chat_receive_packet->type_ = NORMAL;
							chat_receive_packet->nickname_length_ = chat_client_nickname_length;
							chat_receive_packet->header_.size_ = send_message_length;
							for (auto client : chat_clients) {
								client.second->client_->Send((char *)chat_receive_packet, sizeof(PacketHeader) + send_message_length);
							}
						}
					}
				}
			}
			break;
		}
	});
	server.Init();
	server.Run();
	return 0;
}