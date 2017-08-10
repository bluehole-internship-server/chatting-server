#include "ChattingServer.h"

#define GAME_INTERVAL 1000

core::Spinlock lock, command_lock;

int main()
{
	core::Server server_for_game_server;
	core::Client * game_server = nullptr;
	std::thread game_server_thread([&]() {
		server_for_game_server.SetListenPort(55151);
		server_for_game_server.SetPostAcceptHandler([&](core::IoContext * io_context) {
			puts("게임 서버 연결됨.");
			game_server = game_server == nullptr ? io_context->client_ : game_server;
		});
		server_for_game_server.Init();
		puts("게임 서버 시작.");
		server_for_game_server.Run();
	});
		
	core::Server server;

	SetGameCommands(game_commands);

	server.SetListenPort(55150);
	server.SetPostDisconnectHandler([&server](core::IoContext * io_context) {
		auto target = chat_clients.find(io_context->client_);
		if (target == chat_clients.end()) {
			// Do Something.
		}
		else {
			printf("%s 나감.\n", target->second->GetNickname());
			delete target->second;
			chat_clients.erase(io_context->client_);
		}
	});
	server.SetPostReceiveHandler([&](core::IoContext * io_context) {
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
		case LOGIN_REQ: {
			auto login_answer_packet = CreateLoginAnswerPacket(packet_size);
			if (login_answer_packet != nullptr)
			{
				LoginRequestPacket * login_request_packet = (LoginRequestPacket *)buffer;
				char * requested_name = new char[packet_size + 1];
				memcpy(requested_name, login_request_packet->user_name_, packet_size);
				requested_name[packet_size] = 0;
				core::SpinlockGuard lockguard(lock);
				bool is_duplicated = FindDuplicateNickname(client_names, requested_name);
				login_answer_packet->answer_ = is_duplicated ? FAIL_DUPLICATE : SUCCESS;
				if (login_answer_packet->answer_ == SUCCESS) {
					ChatClient * chat_client = new ChatClient();
					SecureZeroMemory(chat_client->GetNickname(), NICKNAME_MAX_LENGTH);
					chat_client->client_ = reciever;
					chat_client->SetNickname(requested_name);
					chat_clients.insert({ reciever ,chat_client });
					client_names.insert(std::string(requested_name));
					printf("%s 접속.\n", chat_client->GetNickname());
				}
			}
			reciever->Send((char *)login_answer_packet, sizeof(LoginAnswerPacket));
		}
		break;
		case CHAT_SEND:
			// Read LockGuard로 변경할 것
			lock.Lock();
			auto target = chat_clients.find(reciever);
			lock.Unlock();
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
					if (is_command_able)
						ProcessCommand(msg, chat_client, packet_size, chat_client_nickname_length);

					unsigned short send_message_length = sizeof(ChatReceivePacket::type_) + sizeof(ChatReceivePacket::nickname_length_) + chat_client_nickname_length + packet_size;
					if (send_message_length > MESSAGE_MAX_LENGTH) {
						// Do Something.
					}
					else {
						ReturnBroadcast(chat_client, chat_client_nickname_length, chat_send_packet, send_message_length, packet_size);
					}
				}
			}
			break;
		}
		delete buffer;
	});
	server.Init();
	server.AddWork([&]() {
		while (true) {
			std::this_thread::sleep_for(std::chrono::milliseconds(GAME_INTERVAL));
			auto picked_command = std::max_element(game_commands.begin(), game_commands.end(),
				[](std::pair<std::string, unsigned int> a, std::pair<std::string, unsigned int> b)->bool { return a.second < b.second; });
			if(picked_command->second != 0)
				printf("선택된 게임 명령어: %s\n", picked_command->first.c_str());
			core::SpinlockGuard  lockguard(command_lock);
			for (auto &command : game_commands) {
				command.second = 0;
			}
			// To Do: Send Picked Command to Logic Server 
			if (game_server) {
				char * sended_command = new char[1];
				memcpy(sended_command, &(game_command_index[picked_command->first]), sizeof(char));
				game_server->Send((char *)sended_command, 1);
				puts("명령어 전송 완료.");
			}
		}
	});
	puts("채팅 서버 시작.");
	server.Run();
	return 0;
}

LoginAnswerPacket * CreateLoginAnswerPacket(unsigned int packet_size)
{
	bool result = true;
	LoginAnswerPacket * login_answer_packet = new LoginAnswerPacket();
	login_answer_packet->answer_ = FAIL_UNKNOWN;
	login_answer_packet->header_.size_ = sizeof(USHORT);
	login_answer_packet->header_.type_ = LOGIN_ANS;

	if (packet_size > NICKNAME_MAX_LENGTH) {
		login_answer_packet->answer_ = FAIL_TOO_LONG;
		result = false;
	}
	else if (packet_size < NICKNAME_MIN_LENGTH) {
		login_answer_packet->answer_ = FAIL_TOO_SHORT;
		result = false;
	}

	if (result == false) {
		delete login_answer_packet;
		return nullptr;
	}
	else {
		return login_answer_packet;
	}
}
bool FindDuplicateNickname(
	std::unordered_set<std::string> &client_names,
	char * requested_name)
{
	std::string requested(requested_name);
	return client_names.find(requested) != client_names.end();
}
ChatReceivePacket * CreateChatReturnPacket(
	unsigned short send_message_length,
	char * nickname,
	unsigned int nickname_length,
	char * message,
	unsigned int message_length)
{
	ChatReceivePacket * return_packet = new ChatReceivePacket();
	return_packet->header_.size_ = send_message_length;
	return_packet->header_.type_ = CHAT_RECV;

	char * return_data = return_packet->data_;

	memcpy(return_data, nickname, nickname_length);
	memcpy(return_data + nickname_length, message, message_length);

	return_packet->type_ = NORMAL;
	return_packet->nickname_length_ = nickname_length;

	return return_packet;
}
void SetGameCommands(std::unordered_map<std::string, unsigned int> &game_commands)
{
	game_commands.insert({ "멈춰", 0 });
	game_commands.insert({ "위", 0 });
	game_commands.insert({ "아래", 0 });
	game_commands.insert({ "왼쪽", 0 });
	game_commands.insert({ "오른쪽", 0 });
}
void ProcessCommand(char * msg, ChatClient * chat_client, unsigned short packet_size, unsigned short chat_client_nickname_length)
{
	char command_type = msg[0];
	char * command_body = nullptr;
	unsigned short command_length = 0;
	switch (command_type) {
	case '/':
		if (packet_size >= 4) {
			char command = msg[1];
			printf("받은 채팅 명령어 %c\n", command);
			command_body = msg + 3;
			command_length = packet_size - 3;
			switch (command) {
			case 'w':
				// Whisper
				ChatClient * sender = chat_client;
				int delimeter_offset = 0;
				for (int i = 0; i < command_length; ++i) {
					if (command_body[i] == ' ') {
						delimeter_offset = i;
					}
				}
				if (delimeter_offset == 0) {
					// Do Something.
				}
				else {
					// Make ChatReceivePacket for Whisper
					unsigned short send_message_length = sizeof(ChatReceivePacket::type_) + sizeof(ChatReceivePacket::nickname_length_) + chat_client_nickname_length + command_length - delimeter_offset - 1;
					if (send_message_length > MESSAGE_MAX_LENGTH) {
						// Do Something.
					}
					else {
						ChatReceivePacket * chat_receive_packet = new ChatReceivePacket();
						chat_receive_packet->header_.size_ = send_message_length;
						chat_receive_packet->header_.type_ = CHAT_RECV;
						chat_receive_packet->type_ = WHISPER;
						chat_receive_packet->nickname_length_ = chat_client_nickname_length;
						core::SpinlockGuard lockguard(lock);
						for (auto client : chat_clients) {
							auto nickname_length = strlen(client.second->GetNickname());
							if (nickname_length == delimeter_offset && memcmp(client.second->GetNickname(), command_body, delimeter_offset) == 0) {
								memcpy(chat_receive_packet->data_, sender->GetNickname(), delimeter_offset);
								memcpy(chat_receive_packet->data_ + delimeter_offset, command_body + delimeter_offset + 1, command_length - delimeter_offset - 1);
								chat_receive_packet->data_[send_message_length] = 0;
								printf("%s --> %s: %s\n", sender->GetNickname(), client.second->GetNickname(), chat_receive_packet->data_ + delimeter_offset);
								client.second->client_->Send((char *)chat_receive_packet, sizeof(PacketHeader) + send_message_length);
								break;
							}
						}
					}
				}
				break;
			}
		}
		break;
	case '!':
	{
		command_body = msg + 1;
		command_length = packet_size - 1;
		printf("받은 게임 명령어 ");
		command_body[command_length] = 0;
		printf("%s\n", command_body);
		std::string command(command_body);
		{
			core::SpinlockGuard lockguard(command_lock);
			auto finder = game_commands.find(command);
			if (finder != game_commands.end())
				++(finder->second);
			else
				printf("알 수 없는 게임 명령어: %s\n", command.c_str());
		}
	}
	break;
	default:
		break;
	}
}

void ReturnBroadcast(ChatClient * chat_client, unsigned short chat_client_nickname_length, ChatSendPacket * chat_send_packet, unsigned short send_message_length, unsigned short packet_size)
{
	printf("%s: ", chat_client->GetNickname());
	for (unsigned int i = 0; i < packet_size; ++i)
		putc(chat_send_packet->message_[i], stdout);
	putc('\n', stdout);
	ChatReceivePacket * return_packet = CreateChatReturnPacket(send_message_length, chat_client->GetNickname(), chat_client_nickname_length, chat_send_packet->message_, packet_size);
	core::SpinlockGuard lockguard(lock);
	for (auto client : chat_clients) {
		client.second->client_->Send((char *)return_packet, sizeof(PacketHeader) + send_message_length);
	}
	delete return_packet;
}
