#pragma once
#include "Server.h"
#include <tlhelp32.h>
#include <psapi.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <wchar.h>
#include <vector>
#include <dshow.h>
#include <ctime>

#pragma comment(lib, "strmiids")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")

#ifndef DEFAULT_BUFLEN
#define DEFAULT_BUFLEN 512
#endif // !DEFAULT_BUFLEN


struct Tasks {
	wchar_t* TaskName = new wchar_t[512];
	wchar_t* TaskDescribe = new wchar_t[512];
};
typedef struct Tasks TASK;
TASK request2TASK(wchar_t*);
int sendStr(SOCKET, const wchar_t*, int len = DEFAULT_BUFLEN);
int recvStr(SOCKET, wchar_t*, int len = DEFAULT_BUFLEN);
int doTasks(SOCKET, TASK);

int ShutdownMachine();
int StartProcess(wchar_t*);
int KillProcess(wchar_t*);
int GetProcessList(wchar_t*);
int listRunningServices(wchar_t*);
int __stdcall DoStopSvc(LPCWSTR);
int __stdcall DoStartSvc(LPCWSTR);
int64_t RecvFile(SOCKET, const wchar_t*, int chunkSize = 64 * 1024);
int64_t SendFile(SOCKET, const wchar_t*, int chunkSize = 64 * 1024);
int WINAPI SaveBitmap(WCHAR*);
int webcam(wchar_t* filename);
