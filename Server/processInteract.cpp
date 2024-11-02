#include "Tasks.h"
//  Forward declarations:

int GetProcessList(wchar_t* fileName)
{
	std::wfstream fp;
	fp.open(fileName, std::ios::out | std::ios::trunc);
	fp << std::setw(40) << std::left << L"Process Name" << std::setw(20) << std::left << L"ProcessID" << std::endl;
	fp << std::setfill(L'-');
	fp << std::setw(60) << L'-' << std::endl;
	fp << std::setfill(L' ');

	HANDLE hProcessSnap;
	HANDLE hProcess;
	PROCESSENTRY32 pe32;

	// Take a snapshot of all processes in the system.
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		fp << L"WARNING: CreateToolhelp32Snapshot (of processes) failed with error";
		return 0;
	}

	// Set the size of the structure before using it.
	pe32.dwSize = sizeof(PROCESSENTRY32);

	// Retrieve information about the first process,
	// and exit if unsuccessful
	if (!Process32First(hProcessSnap, &pe32))
	{
		fp << L"WARNING: Process32First failed with error";
		CloseHandle(hProcessSnap);			// clean the snapshot object
		return 0;
	}

	// Now walk the snapshot of processes, and
	// display information about each process in turn
	do
	{
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
		fp << std::setw(40) << std::left << (pe32.szExeFile) << std::setw(20) << std::hex << (pe32.th32ProcessID) << std::endl;
	} while (Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);
	fp.close();
	return 1;
}
int StartProcess(wchar_t* lpath)
{

	// Display the result and indicate the type of string that it is.
	// additional information
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	// set the size of the structures
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	if (CreateProcess(NULL, lpath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		// Close process and thread handles. 
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return 1;
	}
	return 0;
}

int KillProcess(wchar_t* processName)
{
	HANDLE hProcessSnap;
	HANDLE hProcess;
	PROCESSENTRY32 pe32;

	// Take a snapshot of all processes in the system.
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	// Set the size of the structure before using it.
	pe32.dwSize = sizeof(PROCESSENTRY32);

	// Retrieve information about the first process,
	// and exit if unsuccessful
	if (!Process32First(hProcessSnap, &pe32))
	{
		CloseHandle(hProcessSnap);			// clean the snapshot object
		return 0;
	}

	// Now walk the snapshot of processes, and
	// display information about each process in turn
	do
	{
		// Retrieve the priority class.
		hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
		if (hProcess != NULL) {
			if (wcscmp(processName, pe32.szExeFile) == 0)
			{
				if (TerminateProcess(hProcess, 1)) return 1;
				else return 0;
			}
		}
	} while (Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);
	return 1;
}