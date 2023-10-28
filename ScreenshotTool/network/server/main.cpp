#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#include <optional>

#pragma comment (lib, "Ws2_32.lib")

#include "Packet.h"

void readPacketPayload(Packet& packet, const char* buffer, int size) {
    packet.payload.insert(packet.payload.end(), buffer, buffer + size);
}

std::optional<Packet> readPacket(SOCKET clientSocket) {
    Packet result;

    constexpr size_t bufferLen = 512;
    char buffer[bufferLen];

    bool sizeChunkRead{ false };

    while (true) {
        auto receivedBytes = recv(clientSocket, buffer, bufferLen, 0);
        if (receivedBytes > 0) {
            if (!sizeChunkRead) {
                result.size = *reinterpret_cast<Packet::size_t*>(buffer);
                sizeChunkRead = true;
                readPacketPayload(
                    result, 
                    buffer + sizeof(Packet::size_t), 
                    receivedBytes - sizeof(Packet::size_t));
            }
            else {
                readPacketPayload(result, buffer, receivedBytes);
            }
        }
        else if (receivedBytes == 0) {
            break;
        }
        else {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(clientSocket);
        }
    };

    if (shutdown(clientSocket, SD_SEND) == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(clientSocket);
    }

    closesocket(clientSocket);

    if (result.size == result.payload.size()) {
        return std::move(result);
    }
    return std::nullopt;
}

void handleConnection(SOCKET clientSocket) {
    auto packet = readPacket(clientSocket);
    if (packet) {
        printf("Message: %s\n", packet->payload.data());
    }
}

void mainloop(SOCKET serverSocket) {
    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(serverSocket);
            WSACleanup();
        }
        handleConnection(clientSocket);
    }
}

int main()
{
    WSADATA wsaData;

    SOCKET ListenSocket = INVALID_SOCKET;

    struct addrinfo* address = NULL;
    struct addrinfo hints;

    if (auto result = WSAStartup(MAKEWORD(2, 2), &wsaData); result) {
        printf("WSAStartup failed with error: %d\n", result);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    auto port = "20005";
    if (auto result = getaddrinfo(NULL, port, &hints, &address); result) {
        printf("getaddrinfo failed with error: %d\n", result);
        WSACleanup();
        return 1;
    }

    ListenSocket = socket(
        address->ai_family,
        address->ai_socktype,
        address->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(address);
        WSACleanup();
        return 1;
    }

    auto result = bind(ListenSocket, address->ai_addr, (int)address->ai_addrlen);
    if (result == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(address);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(address);

    result = listen(ListenSocket, SOMAXCONN);
    if (result == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    mainloop(ListenSocket);

    closesocket(ListenSocket);
    WSACleanup();

    return 0;
}