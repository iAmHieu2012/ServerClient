#include "Client.h"

int __cdecl main(int argc, char** argv)
{
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo* result = NULL,
		* ptr = NULL,
		hints;
	int iResult;

	// Validate the parameters
	if (argc != 2)
	{
		printf("usage: %s server-name\n", argv[0]);
		return 1;
	}

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
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
	}
	// Receive until the peer closes the connection

	/*memset(recvbuf, '\0', sizeof(char) * DEFAULT_BUFLEN);
	memset(sendbuf, '\0', sizeof(char) * DEFAULT_BUFLEN);
	fputs("Client: ", stdout);
	fgets(sendbuf, DEFAULT_BUFLEN, stdin);
	sendbuf[strlen(sendbuf) - 1] = '\0';
	iResult = send(ConnectSocket, sendbuf, strlen(sendbuf) + 1, 0);
	iResult = recv(ConnectSocket, recvbuf, DEFAULT_BUFLEN, 0);
	fprintf(stdout, "Server: %s\n", recvbuf);
	if (iResult > 0) {
		if (strcmp(recvbuf, "Machine is shutdown...") == 0
			|| strcmp(recvbuf, "Starting process is finished") == 0
			|| strcmp(recvbuf, "Killing process is finished") == 0
			|| strcmp(recvbuf, "Starting service is finished") == 0
			|| strcmp(recvbuf, "Stopping service is finished") == 0
			|| strcmp(recvbuf, "Failed to start process!") == 0
			|| strcmp(recvbuf, "Failed to kill process!") == 0
			|| strcmp(recvbuf, "Failed to start service!") == 0
			|| strcmp(recvbuf, "Failed to stop process!") == 0) {
			closesocket(ConnectSocket);
			WSACleanup();
			return 0;
		}
		else {
			const int64_t rc = RecvFile(ConnectSocket, std::string(recvbuf), 64 * 1024);
			if (rc < 0) {
				std::cout << "Failed to recv file: " << rc << std::endl;
			}
		}
	}*/
	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}