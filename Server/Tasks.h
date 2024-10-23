#pragma once
#include "Server.h"
#include <tlhelp32.h>
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
int GetProcessList();
int64_t RecvFile(SOCKET, const std::string&, int);
int64_t SendFile(SOCKET, const std::string& , int);
