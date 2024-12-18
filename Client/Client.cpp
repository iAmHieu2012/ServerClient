#include "Client.h"
#include "Mail.h"
#define USER_MAIL "hieughostxyz@gmail.com"
#define CLIENT_MAIL "hieudapchailo@gmail.com"
#define CREDENTIALS_PATH "credentials.json"
std::wstring StringToWString(const std::string& str)
{
	std::wstring wstr;
	size_t size;
	wstr.resize(str.length());
	mbstowcs_s(&size, &wstr[0], wstr.size() + 1, str.c_str(), str.size());
	return wstr;
}
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

	std::string client_id, client_secret, auth_uri;
	getCredentials(CREDENTIALS_PATH, client_id, client_secret, auth_uri);
	char* authorization_code = new char[1024]; // Replace with the authorization code you received
	std::string redirect_uri = "urn:ietf:wg:oauth:2.0:oob"; // Replace with your redirect URI

	std::string access_token;
	std::string refresh_token;
	std::string new_access_token;

	std::string scope = "https://www.googleapis.com/auth/gmail.modify";
	struct memory chunk = { 0 };

	std::string linkAuth = auth_uri + "?" + "client_id=" + client_id + "&redirect_uri=" + redirect_uri + "&response_type=code" + "&scope=" + scope;

	// Replace with your values
	const char* authorization_url = linkAuth.c_str();
	readToken("token.json", access_token, refresh_token);
	if (!checkExistAccessTokenFile("token.json") || access_token == "" || refresh_token == "") {
		// Redirect the user to the authorization URL
		printf("Please visit this URL to authorize the application: %s\n", authorization_url);

		// Wait for the user to authorize and get redirected back
		printf("Enter the authorization code: ");
		fgets(authorization_code, 1024, stdin);

		//Exchange auth code for access token
		if (getAccessTokenFromAuthorizationCode(client_id, client_secret, std::string(authorization_code), redirect_uri, access_token, refresh_token)) {
			// Save access token to file
			saveAccessTokenToFile("token.json", access_token, refresh_token);
		}
		else {
			std::cerr << "Failed to exchange authorization code for access token." << std::endl;
		}
	}

	// Check if the access token is valid
	else {
		if (!isAccessTokenValid(access_token)) {
			std::cout << "Access token is invalid or expired. Refreshing token...\n";
			if (refreshAccessToken(client_id, client_secret, refresh_token, new_access_token)) {
				access_token = new_access_token;  // Update the access token
				saveAccessTokenToFile("token.json", access_token, refresh_token);
			}
			else {
				std::cerr << "Failed to refresh the access token.\n";
			}
		}
	}

	// Now you can use the valid (or refreshed) access_token to make API requests
	std::cout << "Using access token: " << access_token << std::endl;
	////////////////////////////////////////////
	while (true) {
		system("cls");
		std::cout << "Checking for new messages..." << std::endl;

		std::vector<std::vector<std::string>> IP_tasks = getUnreadMessageContents(access_token);
		for (const std::vector<std::string>& messageContent : IP_tasks) {
			std::cout << "New Message Subject: " << messageContent[0] << std::endl;
			std::cout << "New Message Content: " << messageContent[1] << std::endl;
			std::cout << "New Message Sender address: " << messageContent[2] << std::endl;
			if (messageContent[2] != USER_MAIL) continue;
			struct addrinfo* result = NULL,
				* ptr = NULL,
				hints;
			int iResult;
			char* ipaddr = new char[100];
			strcpy(ipaddr, messageContent[0].c_str());
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
			SOCKET ConnectSocket = INVALID_SOCKET;
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
			fputws(L"Client: ", stdout);
			wcscpy(sendbuf, StringToWString(messageContent[1]).c_str());
			//fgetws(sendbuf, DEFAULT_BUFLEN, stdin);
			if (wcslen(sendbuf) == 0) {
				continue;
			}
			if (wcscmp(sendbuf, L"TURNONCAMERA") >= 0) {
				time_t now = time(0);
				wchar_t filename[100];
				swprintf(filename, 100, L"webcam_%lld.avi", (long long)now);
				wcscpy(sendbuf, (L"TURNONCAMERA " + std::wstring(filename)).c_str());
			}
			std::wcout << sendbuf << std::endl;
			iResult = send(ConnectSocket, reinterpret_cast<const char*>(sendbuf), DEFAULT_BUFLEN, 0);
			if (iResult > 0) {
				TASK t = request2TASK(sendbuf);
				std::string sender_email = CLIENT_MAIL;
				std::string recipient_email = USER_MAIL;
				std::string subject = "Respone from server " + messageContent[0];

				if (wcscmp(t.TaskName, L"LISTPROCESS") == 0
					|| wcscmp(t.TaskName, L"LISTSERVICES") == 0
					|| wcscmp(t.TaskName, L"SENDFILE") == 0
					|| wcscmp(t.TaskName, L"SCREENCAPTURE") == 0
					|| wcscmp(t.TaskName, L"TURNONCAMERA") == 0)
				{

					std::string body = "The file is generated";
					std::wstring temp(t.TaskDescribe);
					std::string file_path = std::string(temp.begin(), temp.end());
					std::string file_name = FileName(file_path);
					const int64_t rc = RecvFile(ConnectSocket, FileName(temp).c_str());
					if (rc < 0)
					{
						std::cout << "Failed to recv file: " << rc << std::endl;
						try {
							if (sendEmailViaGmailAPI(access_token, sender_email, recipient_email, subject, "Failed to get the file from server")) {
								std::cout << "Email sent successfully!" << std::endl;
							}
							else {
								std::cerr << "Failed to send email." << std::endl;
							}
						}
						catch (const std::exception& e) {
							std::cerr << "Error: " << e.what() << std::endl;
						}
					}
					else {
						iResult = recvStr(ConnectSocket, recvbuf);
						if (iResult < 0) std::cout << "Failed to recv str: " << rc << std::endl;
						else {
							try {
								if (sendEmailWithAttachment(access_token, sender_email, recipient_email, subject, body, file_path, file_name)) {
									std::cout << "Email sent successfully!" << std::endl;
								}
								else {
									std::cerr << "Failed to send email." << std::endl;
								}
							}
							catch (const std::exception& e) {
								std::cerr << "Error: " << e.what() << std::endl;
							}
						}
					}
					
				}
				else {

					iResult = recvStr(ConnectSocket, recvbuf);
					if (iResult < 0) continue;
					else {
						try {
							if (sendEmailViaGmailAPI(access_token, sender_email, recipient_email, subject, "Done")) {
								std::cout << "Email sent successfully!" << std::endl;
							}
							else {
								std::cerr << "Failed to send email." << std::endl;
							}
						}
						catch (const std::exception& e) {
							std::cerr << "Error: " << e.what() << std::endl;
						}
					}
					std::wcout << L"Server: " << recvbuf << std::endl;
				}
			}
			else if (iResult == 0) {
				printf("Closing connecting to IP %s...\n", ipaddr);
			}
			else {
				printf("Something went wrong!?...\n");
				printf("recv failed with error: %d\n", WSAGetLastError());
				closesocket(ConnectSocket);
				WSACleanup();
				continue;
			}
			closesocket(ConnectSocket);
			// Wait for the specified interval before checking again
		}
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
	WSACleanup();

	return 0;

}
