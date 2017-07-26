#include <Server.h>

int main()
{
	core::Server server;
	server.SetListenPort(5150);
	server.SetReceiveHandler([&server](core::IoContext * io_context) {
		core::Client * reciever = io_context->client_;
		reciever->recv_buffer_.SetHead(0);
		auto recieved = io_context->received_;
		char * buffer = new char[recieved];
		memcpy(buffer, reciever->recv_buffer_.Read(), recieved);
		auto clients = server.GetAllClient();
		for (auto c : clients) {
			c->Send(buffer, recieved);
		}
	});
	server.Init();
	server.Run();
	return 0;
}