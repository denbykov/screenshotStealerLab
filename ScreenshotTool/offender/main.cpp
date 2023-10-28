#include <windows.h>

#include <iostream>
#include <stdint.h>

//DWORD hook_address = 0x00007FF716095CB0;
//DWORD ret_address = 0x00007FF7BE545CB6;
//DWORD hook_address = 0x00007FF716095CB0;
//uint64_t hook_address = 0x00007FF7488B5CB0; // address when attaching with the x64dgb
//DWORD hook_address = 0x00007FF798FC5CB0; // address when starting in the x64dgb

// started in x64dbg
// 00007FF711230000 base 
// 00007FF711245C10 make screenshot
// diff 15C10

// attached with x64dbg
// 00007FF673110000 base
// 00007FF673125C10 make screenshot
// diff 15C10

// offset is stable!


uint64_t offset = 0x15CB0;

BYTE nop = 0x90;

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

    return (uint64_t)handle + offset;
}

void hookCode() {
    auto hook_location = (char*)getHookLocation();
    
    DWORD old_protect;
    auto res = VirtualProtect((void*)hook_location, 6, PAGE_EXECUTE_READWRITE, &old_protect);
    if (res == 0) {
        std::cout << "virtual protect failed" << std::endl;

        printLastError();
        return;
    }

    *(hook_location) = nop;
    *(hook_location + 1) = nop;
    *(hook_location + 2) = nop;
    *(hook_location + 3) = nop;
    *(hook_location + 4) = nop;
    *(hook_location + 5) = nop;
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