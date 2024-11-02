#pragma once
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <conio.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

struct Tasks {
	wchar_t* TaskName = new wchar_t[512];
	wchar_t* TaskDescribe = new wchar_t[512];
};
typedef struct Tasks TASK;
TASK request2TASK(wchar_t*);
int64_t RecvFile(SOCKET, const wchar_t*, int chunkSize = 64 * 1024);
int64_t SendFile(SOCKET, const wchar_t*, int chunkSize = 64 * 1024);
int sendStr(SOCKET, const wchar_t*, int len = DEFAULT_BUFLEN);
int recvStr(SOCKET, wchar_t*, int len = DEFAULT_BUFLEN);
