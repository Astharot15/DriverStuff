// interactDriver.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <errhandlingapi.h>
#include "../KMDF Driver1/ioctl.h"

int main(int agrc, char* argv[])
{
	HANDLE hDriver;
	BOOL status;
	DWORD pid = (DWORD)atoi(argv[1]);

	hDriver = CreateFile(
		L"\\\\.\\TestDriver",
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		0,
		nullptr
	);

	if (hDriver == INVALID_HANDLE_VALUE) {
		printf("Failed to open handle: %d\n", GetLastError());
		return 1;
	}

	printf("pid: %d", pid);

	if (pid == 0) {
		printf("Something went wrong %d\n", GetLastError());
	}

	status = DeviceIoControl(
		hDriver,
		PROCESS_PERMISSION,
		&pid,
		sizeof(pid),
		nullptr,
		0,
		nullptr,
		nullptr
	);

	if (status) {
		printf("Call worked as intended\n");
	}
	else {
		printf("Failed with error code %d\n", GetLastError());
	}

	printf("Closing handle\n");
	CloseHandle(hDriver);
}

