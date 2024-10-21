#pragma once
#include "Server.h"
#include <stdio.h>
#include <algorithm>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
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