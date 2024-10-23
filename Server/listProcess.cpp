#include "Tasks.h"
//  Forward declarations:
int GetProcessList();
void printError(TCHAR const* msg);

int GetProcessList()
{
	std::fstream fp;
	fp.open("process.txt", std::ios::out | std::ios::trunc);
	fp << std::setw(40) << std::left << "Process Name" << std::setw(20) << std::left << "ProcessID" << std::endl;
	fp << std::setfill('-');
	fp << std::setw(60) << '-' << std::endl;
	fp << std::setfill(' ');

	HANDLE hProcessSnap;
	HANDLE hProcess;
	PROCESSENTRY32 pe32;

	// Take a snapshot of all processes in the system.
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		printError(TEXT("CreateToolhelp32Snapshot (of processes)"));
		return 0;
	}

	// Set the size of the structure before using it.
	pe32.dwSize = sizeof(PROCESSENTRY32);

	// Retrieve information about the first process,
	// and exit if unsuccessful
	if (!Process32First(hProcessSnap, &pe32))
	{
		printError(TEXT("Process32First")); // show cause of failure
		CloseHandle(hProcessSnap);			// clean the snapshot object
		return 0;
	}

	// Now walk the snapshot of processes, and
	// display information about each process in turn
	do
	{
		_tprintf(TEXT("\n\n====================================================="));
		_tprintf(TEXT("\nPROCESS NAME:  %s"), pe32.szExeFile);
		_tprintf(TEXT("\n-------------------------------------------------------"));
		// Retrieve the priority class.
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
		if (hProcess == NULL)
			printError(TEXT("OpenProcess"));

		_tprintf(TEXT("\n  Process ID        = 0x%08X"), pe32.th32ProcessID);
		// _tprintf(TEXT("\n  Thread count      = %d"), pe32.cntThreads);
		// _tprintf(TEXT("\n  Parent process ID = 0x%08X"), pe32.th32ParentProcessID);
		fp << std::setw(40) << std::left << (pe32.szExeFile) << std::setw(20) << std::hex << (pe32.th32ProcessID) << std::endl;
	} while (Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);
	return 1;
}

void printError(TCHAR const* msg)
{
	DWORD eNum;
	TCHAR sysMsg[256];
	TCHAR* p;

	eNum = GetLastError();
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, eNum,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		sysMsg, 256, NULL);

	// Trim the end of the line and terminate it with a null
	p = sysMsg;
	while ((*p > 31) || (*p == 9))
		++p;
	do
	{
		*p-- = 0;
	} while ((p >= sysMsg) &&
		((*p == '.') || (*p < 33)));

	// Display the message
	_tprintf(TEXT("\n  WARNING: %s failed with error %d (%s)"), msg, eNum, sysMsg);
}