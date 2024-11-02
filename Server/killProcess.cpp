#include "Tasks.h"

int KillProcess(wchar_t* processName)
{
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
		// Retrieve the priority class.
		hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
		if (hProcess == NULL)
			printError(TEXT("OpenProcess"));

		if (wcscmp(processName,pe32.szExeFile)==0)
		{
			if (TerminateProcess(hProcess, 1)) return 1;
			else return 0;
		}
	} while (Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);
	return 1;
}