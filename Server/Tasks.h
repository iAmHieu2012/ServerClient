#pragma once
#include "Server.h"
#include "camera.h"
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
#include <tchar.h>
#include <wchar.h>
#include <vector>
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")

struct Tasks {
	wchar_t* TaskName = new wchar_t[512];
	wchar_t* TaskDescribe = new wchar_t[512];
};
typedef struct Tasks TASK;
TASK request2TASK(char*);
int doTasks(SOCKET, TASK);

int ShutdownMachine();
int StartProcess(wchar_t*);
int KillProcess(wchar_t*);
void printError(TCHAR const* msg);
int GetProcessList(wchar_t*);
int listRunningServices(wchar_t*);
int __stdcall DoStopSvc(LPCWSTR);
int __stdcall DoStartSvc(LPCWSTR);
int64_t RecvFile(SOCKET, const std::string&, int);
int64_t SendFile(SOCKET, const std::string& , int);
int WINAPI SaveBitmap(WCHAR*);
int WINAPI turnOnCamera(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

