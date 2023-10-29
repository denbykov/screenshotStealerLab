#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <stdio.h>
#include <objidl.h>
#include <gdiplus.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <optional>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment(lib, "gdiplus.lib")

#include "Packet.h"

HBITMAP makeScreenshot() {
    HDC screenDC = GetDC(NULL);

    HDC memDC = CreateCompatibleDC(screenDC);

    int screenX = GetSystemMetrics(SM_CXSCREEN);
    int screenY = GetSystemMetrics(SM_CYSCREEN);

    HBITMAP hBitmap = CreateCompatibleBitmap(screenDC, screenX, screenY);

    SelectObject(memDC, hBitmap);

    BitBlt(memDC, 0, 0, screenX, screenY, screenDC, 0, 0, SRCCOPY);

    DeleteDC(memDC);
    ReleaseDC(NULL, screenDC);

    return hBitmap;
}

std::optional<Packet> BitmapToPacket(HBITMAP hBitmap) {
    Packet result;

    BITMAP bmp;
    if (!GetObject(hBitmap, sizeof(BITMAP), &bmp))
        return std::nullopt;

    BITMAPINFOHEADER bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmp.bmWidth;
    result.biWidth = bmp.bmWidth;
    bi.biHeight = bmp.bmHeight;
    result.biHeight = bmp.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = bmp.bmBitsPixel;
    result.biBitCount = bmp.bmBitsPixel;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    DWORD dwBmpSize = ((bmp.bmWidth * bmp.bmBitsPixel + 31) / 32) * 4 * bmp.bmHeight;
    result.payload = (BYTE*)malloc(dwBmpSize);

    HDC hDC = GetDC(NULL);
    if (hDC == NULL)
        return std::nullopt;

    if (!GetDIBits(hDC, hBitmap, 0, bmp.bmHeight, result.payload, (BITMAPINFO*)&bi, DIB_RGB_COLORS)) {
        ReleaseDC(NULL, hDC);
        free(result.payload);
        return std::nullopt;
    }

    ReleaseDC(NULL, hDC);
    result.payloadSize = dwBmpSize;
    return result;
}

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

bool send(SOCKET socket, Packet::size_t data) {
    auto result = send(
        socket,
        reinterpret_cast<const char*>(&data),
        sizeof(Packet::size_t),
        0);

    if (result == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(socket);
        return false;
    }

    return true;
}

void send(SOCKET socket, Packet& packet) {
    if (socket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        return;
    }
    
    if (!send(socket, packet.biWidth) ||
        !send(socket, packet.biHeight) ||
        !send(socket, packet.biBitCount) ||
        !send(socket, packet.payloadSize)) {
        return;
    }

    int bytesSend = 0;

    const char* payload = reinterpret_cast<char*>(packet.payload);

    while (bytesSend != packet.payloadSize) {

        auto result = send(socket, payload, packet.payloadSize, 0);
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

    auto screenshot = makeScreenshot();

    if (auto packet = BitmapToPacket(screenshot); packet) {
        printf("biWidth: %d\n", packet->biWidth);
        printf("biHeight: %d\n", packet->biHeight);
        printf("biBitCount: %d\n", packet->biBitCount);
        printf("payloadSize: %d\n", packet->payloadSize);
        send(connect(), *packet);
        free(packet->payload);
    }

    WSACleanup();
     
    return 0;
}