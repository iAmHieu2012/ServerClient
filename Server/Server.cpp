#include "Server.h"
#include "Tasks.h"
int __cdecl main(void)
{
	WSADATA wsaData;
	int iResult;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}
	else printf("WSAStartup has been initialized successfully\n");


	struct addrinfo* result = NULL;
	struct addrinfo hints;


	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0)
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		return 1;
	}
	int shutdownServerApplication = 0;
	while (shutdownServerApplication == 0) {

		// Create a SOCKET for the server to listen for client connections.
		SOCKET ListenSocket = INVALID_SOCKET;
		ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (ListenSocket == INVALID_SOCKET)
		{
			printf("socket failed with error: %ld\n", WSAGetLastError());
			freeaddrinfo(result);
			return 1;
		}

		// Setup the TCP listening socket
		iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			printf("bind failed with error: %d\n", WSAGetLastError());
			freeaddrinfo(result);
			closesocket(ListenSocket);
			return 1;
		}

		iResult = listen(ListenSocket, SOMAXCONN);
		if (iResult == SOCKET_ERROR)
		{
			printf("listen failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			return 1;
		}

		// Accept a client socket
		SOCKET ClientSocket = INVALID_SOCKET;
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET)
		{
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			return 1;
		}

		// No longer need server socket
		closesocket(ListenSocket);

		// Receive until the peer shuts down the connection
		wchar_t* sendbuf = new wchar_t[DEFAULT_BUFLEN];
		wchar_t* recvbuf = new wchar_t[DEFAULT_BUFLEN];
		iResult = recv(ClientSocket, reinterpret_cast<char*>(recvbuf), DEFAULT_BUFLEN, 0);
		TASK t = request2TASK(recvbuf);
		if (iResult > 0) {
			std::wcout << "Client: " << recvbuf << std::endl;
			if (wcscmp(t.TaskName, L"END") == 0) {
				shutdownServerApplication = 1;
			}
			iResult = doTasks(ClientSocket, t);
			if (iResult <= 0) continue;
		}
		else if (iResult == 0) {
			printf("Connection closing...\n");
		}
		else {
			printf("Something went wrong!?...\n");
			printf("recv failed with error: %d\n", WSAGetLastError());
		}
		// cleanup
		closesocket(ClientSocket);
	}
	WSACleanup();
	return 0;
}