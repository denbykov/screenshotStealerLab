#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment (lib, "Ws2_32.lib")

#include "Packet.h"

SOCKET connect() {
    SOCKET connectSocket = INVALID_SOCKET;
    struct addrinfo* address = NULL;
    struct addrinfo hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    auto port = "20005";
    if (auto result = getaddrinfo(NULL, port, &hints, &address); result) {
        printf("getaddrinfo failed with error: %d\n", result);
        return INVALID_SOCKET;
    }

    for (struct addrinfo* ptr = address; ptr != NULL; ptr = ptr->ai_next) {
        connectSocket = socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (connectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            return INVALID_SOCKET;
        }

        auto result = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (result == SOCKET_ERROR) {
            closesocket(connectSocket);
            connectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(address);

    return connectSocket;
}

void send(SOCKET socket, Packet& packet) {
    if (socket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        return;
    }
    
    {
        auto result = send(
            socket,
            reinterpret_cast<const char*>(&packet.size),
            sizeof(Packet::size_t),
            0);

        if (result == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(socket);
            return;
        }
    }

    int bytesSend = 0;

    const char* payload = packet.payload;

    while (bytesSend != packet.size) {

        auto result = send(socket, payload, packet.size, 0);
        if (result == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(socket);
            return;
        }

        bytesSend += result;
        payload += result;
    }

    if (auto result = shutdown(socket, SD_SEND);  result == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(socket);
        return;
    }

    closesocket(socket);
}

int main()
{
    WSADATA wsaData;
    if (auto result = WSAStartup(MAKEWORD(2, 2), &wsaData); result) {
        printf("WSAStartup failed with error: %d\n", result);
        return 1;
    }
    
    const char* message = "Hello tcp server!";

    Packet packet;
    packet.payload = message;
    packet.size = strlen(message) + 1;

    send(connect(), packet);

    WSACleanup();

    return 0;
}