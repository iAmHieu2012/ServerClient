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
	a.TaskDescribe[wcslen(nextToken) + 1] = '\0';
	return a;
}
int doTasks(SOCKET ClientSocket, TASK a) {
	int res = 0;
	if (wcscmp(a.TaskName,L"SHUTDOWN") == 0) {
		char* recvbuf = new char[DEFAULT_BUFLEN];
		strcpy_s(recvbuf, sizeof(recvbuf), "Task is finished");
		int iSentResult = 0;
		iSentResult = send(ClientSocket, recvbuf, strlen(recvbuf) + 1, 0);
		printf("Connection closing...\n");
		closesocket(ClientSocket);
		WSACleanup();
		res = ShutdownMachine();
	}
	else if (wcscmp(a.TaskName, L"STARTPROCESS") == 0) {
		res = StartProcess(a.TaskDescribe);
	}
	else if (wcscmp(a.TaskName, L"LISTPROCESS") == 0) {
		res = GetProcessList();
		int64_t iSendResult = 0;
		iSendResult = SendFile(ClientSocket, "process.txt",);
	}
	return res;
}