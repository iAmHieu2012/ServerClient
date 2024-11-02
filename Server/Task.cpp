#include "Server.h"
#include "Tasks.h"

TASK request2TASK(char* request) {
	TASK a;
	size_t newsize = strlen(request) + 1;
	wchar_t* wcRequest = new wchar_t[newsize];
	size_t convertedChars = 0;
	mbstowcs_s(&convertedChars, wcRequest, newsize, request, _TRUNCATE);
	wchar_t* nextToken = NULL;
	a.TaskName = wcstok_s(wcRequest, L" ", &nextToken);
	a.TaskDescribe = nextToken;
	return a;
}
int doTasks(SOCKET ClientSocket, TASK a) {
	int res = 0;
	char* sendbuf = new char[DEFAULT_BUFLEN];
	memset(sendbuf, '\0', DEFAULT_BUFLEN);
	int iSentResult = 0;
	if (wcscmp(a.TaskName, L"SHUTDOWN") == 0) {
		strcpy_s(sendbuf, DEFAULT_BUFLEN, "Machine is shutdown...");
		iSentResult = send(ClientSocket, sendbuf, strlen(sendbuf) + 1, 0);
		res = ShutdownMachine();
	}
	else if (wcscmp(a.TaskName, L"STARTPROCESS") == 0) {
		res = StartProcess(a.TaskDescribe);
		if (res) strcpy_s(sendbuf, DEFAULT_BUFLEN, "Starting process is finished");
		else strcpy_s(sendbuf, DEFAULT_BUFLEN, "Failed to start process!");
		iSentResult = send(ClientSocket, sendbuf, strlen(sendbuf) + 1, 0);
	}
	else if (wcscmp(a.TaskName, L"KILLPROCESS") == 0) {
		res = KillProcess(a.TaskDescribe);
		if (res) strcpy_s(sendbuf, DEFAULT_BUFLEN, "Killing process is finished");
		else strcpy_s(sendbuf, DEFAULT_BUFLEN, "Failed to kill process!");
		iSentResult = send(ClientSocket, sendbuf, strlen(sendbuf) + 1, 0);
	}
	else if (wcscmp(a.TaskName, L"LISTPROCESS") == 0) {
		res = GetProcessList(a.TaskDescribe);
		char* buffer = new char[DEFAULT_BUFLEN];
		size_t convertedChars = 0;
		wcstombs_s(&convertedChars, buffer, DEFAULT_BUFLEN, a.TaskDescribe, DEFAULT_BUFLEN);
		std::string path(buffer);
		strcpy_s(sendbuf, DEFAULT_BUFLEN, buffer);
		iSentResult = send(ClientSocket, sendbuf, strlen(sendbuf) + 1, 0);
		if (res==1) {
			int64_t rc = SendFile(ClientSocket, path, 64 * 1024);
			if (rc < 0) return -1;
		}
	}
	else if (wcscmp(a.TaskName, L"LISTSERVICES") == 0) {
		res = listRunningServices(a.TaskDescribe);
		char* buffer = new char[DEFAULT_BUFLEN];
		size_t convertedChars = 0;
		wcstombs_s(&convertedChars, buffer, DEFAULT_BUFLEN, a.TaskDescribe, DEFAULT_BUFLEN);
		std::string path(buffer);
		strcpy_s(sendbuf, DEFAULT_BUFLEN, buffer);
		iSentResult = send(ClientSocket, sendbuf, strlen(sendbuf) + 1, 0);
		if (res == 1) {
			int64_t rc = SendFile(ClientSocket, path, 64 * 1024);
			if (rc < 0) return -1;
		}
	}
	else if (wcscmp(a.TaskName, L"STARTSERVICE") == 0) {
		res = DoStartSvc(a.TaskDescribe);
		if (res) strcpy_s(sendbuf, DEFAULT_BUFLEN, "Starting service is finished");
		else strcpy_s(sendbuf, DEFAULT_BUFLEN, "Failed to start service!");
		iSentResult = send(ClientSocket, sendbuf, strlen(sendbuf) + 1, 0);
	}
	else if (wcscmp(a.TaskName, L"STOPSERVICE") == 0) {
		res = DoStopSvc(a.TaskDescribe);
		if (res) strcpy_s(sendbuf, DEFAULT_BUFLEN, "Stopping service is finished");
		else strcpy_s(sendbuf, DEFAULT_BUFLEN, "Failed to stop service!");
		iSentResult = send(ClientSocket, sendbuf, strlen(sendbuf) + 1, 0);
	}
	else if (wcscmp(a.TaskName, L"SENDFILE") == 0) {
		char* buffer = new char[DEFAULT_BUFLEN];
		size_t convertedChars = 0;
		wcstombs_s(&convertedChars, buffer, DEFAULT_BUFLEN, a.TaskDescribe, DEFAULT_BUFLEN);
		std::string path(buffer);
		strcpy_s(sendbuf, DEFAULT_BUFLEN, buffer);
		iSentResult = send(ClientSocket, sendbuf, strlen(sendbuf) + 1, 0);
		int64_t rc = SendFile(ClientSocket, path, 64 * 1024);
		if (rc < 0) return -1;
	}
	else if (wcscmp(a.TaskName, L"SCREENCAPTURE") == 0) {
		res = SaveBitmap(a.TaskDescribe);
		char* buffer = new char[DEFAULT_BUFLEN];
		size_t convertedChars = 0;
		wcstombs_s(&convertedChars, buffer, DEFAULT_BUFLEN, a.TaskDescribe, DEFAULT_BUFLEN);
		std::string path(buffer);
		strcpy_s(sendbuf, DEFAULT_BUFLEN, buffer);
		iSentResult = send(ClientSocket, sendbuf, strlen(sendbuf) + 1, 0);
		if (res == 1) {
			int64_t rc = SendFile(ClientSocket, path, 64 * 1024);
			if (rc < 0) return -1;
		}
	}
	return res;
}