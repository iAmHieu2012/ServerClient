#include "Client.h"

int __cdecl main(void)
{
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo* result = NULL,
		* ptr = NULL,
		hints;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}
	char* ipaddr = new char[100];
	std::cout << "Enter IP:" << std::endl;
	std::cin >> ipaddr;
	std::cin.clear();
	std::cin.ignore();
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(ipaddr, DEFAULT_PORT, &hints, &result);
	if (iResult != 0)
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET)
		{
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET)
	{
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	wchar_t* sendbuf = new wchar_t[DEFAULT_BUFLEN];
	wchar_t* recvbuf = new wchar_t[DEFAULT_BUFLEN];
	while (1) {
		fputws(L"Client: ", stdout);
		fgetws(sendbuf, DEFAULT_BUFLEN, stdin);
		sendbuf[wcslen(sendbuf) - 1] = L'\0';
		if (wcslen(sendbuf) == 0) {
			break;
		}
		iResult = send(ConnectSocket, reinterpret_cast<const char*>(sendbuf), DEFAULT_BUFLEN, 0);
		if (iResult > 0) {
			TASK t = request2TASK(sendbuf);
			if (wcscmp(t.TaskName, L"LISTPROCESS") == 0
				|| wcscmp(t.TaskName, L"LISTSERVICES") == 0
				|| wcscmp(t.TaskName, L"SENDFILE") == 0
				|| wcscmp(t.TaskName, L"SCREENCAPTURE") == 0)
			{
				const int64_t rc = RecvFile(ConnectSocket, t.TaskDescribe);
				if (rc < 0)
				{
					std::cout << "Failed to recv file: " << rc << std::endl;
				}
			}
			else if (wcscmp(t.TaskName, L"END") == 0 || wcscmp(t.TaskName, L"SHUTDOWN") == 0)
			{
				break;
			}
			iResult = recvStr(ConnectSocket, recvbuf);
			std::wcout << L"Server: " << recvbuf << std::endl;
		}
		else if (iResult == 0) {
			printf("Connection closing...\n");
			break;
		}
		else {
			printf("Something went wrong!?...\n");
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();
			return 1;
		}
	}
	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}