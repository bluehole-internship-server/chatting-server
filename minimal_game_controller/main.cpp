//
// main.cpp
//

#include <iostream>

#define _WINSOCKAPI_

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

bool init_wsa()
{
    WSAData wsa_data;
    return WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0;
}

int main()
{
    init_wsa();
    uint8_t command;
    int opt = 1;

    SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in server_addr;

    setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(55151);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    ::bind(socket, (sockaddr*)&server_addr, sizeof(server_addr));
    ::listen(socket, 5);
    
    SOCKET game_server = accept(socket, nullptr, nullptr);
    std::cout << "Game server connected\n";

    while (true) {
        int temp;
        std::cout << "Command : ";
        std::cin >> temp;
        command = temp;
        send(game_server, (const char*)&command, sizeof(command), 0);
        Sleep(1000);
    }
    
}