#include "Server.h"
#include "Tasks.h"

int sendStr(SOCKET s, const wchar_t* buf, int len) {
	return send(s, reinterpret_cast<const char*>(buf), len, 0);
}
int recvStr(SOCKET s, wchar_t* buf, int len) {
	return recv(s, reinterpret_cast<char*>(buf), len, 0);
}

TASK request2TASK(wchar_t* request) {
	TASK a;
	wchar_t* tmp = new wchar_t[DEFAULT_BUFLEN];
	wcscpy_s(tmp, DEFAULT_BUFLEN, request);
	wchar_t* nextToken = NULL;
	a.TaskName = wcstok_s(tmp, L" ", &nextToken);
	a.TaskDescribe = nextToken;
	return a;
}
int doTasks(SOCKET ClientSocket, TASK a) {
	int res = 1;
	int iSentResult = 1;
	if (wcscmp(a.TaskName, L"SHUTDOWN") == 0) {
		iSentResult = sendStr(ClientSocket, L"Machine is shutdown...");
		res = 0;
		res = ShutdownMachine();
	}

	else if (wcscmp(a.TaskName, L"STARTPROCESS") == 0) {
		res = StartProcess(a.TaskDescribe);
		if (res) iSentResult = sendStr(ClientSocket, L"Starting process is finished");
		else iSentResult = sendStr(ClientSocket, L"Failed to start process!");
		if (iSentResult <= 0) res = -1;
	}

	else if (wcscmp(a.TaskName, L"KILLPROCESS") == 0) {
		res = KillProcess(a.TaskDescribe);
		if (res) iSentResult = sendStr(ClientSocket, L"Killing process is finished");
		else iSentResult = sendStr(ClientSocket, L"Failed to kill process!");
		if (iSentResult <= 0) res = -1;
	}

	else if (wcscmp(a.TaskName, L"LISTPROCESS") == 0) {
		res = GetProcessList(a.TaskDescribe);
		if (res == 1) {
			int64_t rc = SendFile(ClientSocket, a.TaskDescribe);
			if (rc < 0) return -1;
			else sendStr(ClientSocket, L"File is sent!");
		}
	}

	else if (wcscmp(a.TaskName, L"LISTSERVICES") == 0) {
		res = listRunningServices(a.TaskDescribe);
		if (res == 1) {
			int64_t rc = SendFile(ClientSocket, a.TaskDescribe);
			if (rc < 0) return -1;
			else sendStr(ClientSocket, L"File is sent!");
		}
	}

	else if (wcscmp(a.TaskName, L"STARTSERVICE") == 0) {
		res = DoStartSvc(a.TaskDescribe);
		if (res) iSentResult = sendStr(ClientSocket, L"Starting service is finished");
		else iSentResult = sendStr(ClientSocket, L"Failed to start service!");
		if (iSentResult <= 0) res = -1;
	}

	else if (wcscmp(a.TaskName, L"STOPSERVICE") == 0) {
		res = DoStopSvc(a.TaskDescribe);
		if (res) iSentResult = sendStr(ClientSocket, L"Stopping service is finished");
		else iSentResult = sendStr(ClientSocket, L"Failed to stop service!");
		if (iSentResult <= 0) res = -1;
	}

	else if (wcscmp(a.TaskName, L"SENDFILE") == 0) {
		int64_t rc = SendFile(ClientSocket, a.TaskDescribe);
		if (rc < 0) return -1;
		else sendStr(ClientSocket, L"File is sent!");
	}

	else if (wcscmp(a.TaskName, L"SCREENCAPTURE") == 0) {
		res = SaveBitmap(a.TaskDescribe);
		if (res == 1) {
			int64_t rc = SendFile(ClientSocket, a.TaskDescribe);
			if (rc < 0) return -1;
			else sendStr(ClientSocket, L"File is sent!");
		}
	}
	else if (wcscmp(a.TaskName, L"TURNONCAMERA") == 0) {
		res = webcam(a.TaskDescribe);
		if (res == 1) {
			int64_t rc = SendFile(ClientSocket, a.TaskDescribe);
			if (rc < 0) return -1;
			else sendStr(ClientSocket, L"File is sent!");
		}
	}
	else sendStr(ClientSocket, L"_unable to do the task_");
	return res;
}