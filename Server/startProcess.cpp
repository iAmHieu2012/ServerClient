#include "Tasks.h"

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
