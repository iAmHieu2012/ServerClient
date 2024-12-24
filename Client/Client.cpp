#define _CRT_SECURE_NO_WARNINGS
#include "Mail.h"
#include "Client.h"

#define ID_AUTHCODE 101
#define ID_BUTTON 103
#define ID_BUTTONLINK 102
#define ID_BUTTONMAIL 105
#define ID_OUTPUT 104
#define ID_USERMAIL 201
#define ID_SUBMITMAIL 202
#define CLIENT_MAIL "hieudapchailo@gmail.com"//immutable 
#define CREDENTIALS_PATH "credentials.json"//immutable
std::string USER_MAIL = "hieughostxyz@gmail.com";
std::string wcharToString(const wchar_t* wstr) {
	size_t len = std::wcslen(wstr) + 1;
	char* buffer = new char[len];
	std::wcstombs(buffer, wstr, len);
	std::string str(buffer);
	delete[] buffer;
	return str;
}
std::wstring StringToWString(const std::string& str)
{
	std::wstring wstr;
	size_t size;
	wstr.resize(str.length());
	mbstowcs_s(&size, &wstr[0], wstr.size() + 1, str.c_str(), str.size());
	return wstr;
}
std::string Authlink(std::string& client_id, std::string& client_secret, std::string& auth_uri) {
	getCredentials(CREDENTIALS_PATH, client_id, client_secret, auth_uri);
	std::string redirect_uri = "urn:ietf:wg:oauth:2.0:oob"; // Replace with your redirect URI

	std::string scope = "https://www.googleapis.com/auth/gmail.modify";

	return auth_uri + "?" + "client_id=" + client_id + "&redirect_uri=" + redirect_uri + "&response_type=code" + "&scope=" + scope;
}

std::string getAccessToken(char*& authorization_code) {

	std::string client_id, client_secret, auth_uri;
	Authlink(client_id, client_secret, auth_uri);
	std::string redirect_uri = "urn:ietf:wg:oauth:2.0:oob"; // Replace with your redirect URI

	std::string access_token;
	std::string refresh_token;
	std::string new_access_token;

	// Replace with your values
	readToken("token.json", access_token, refresh_token);
	if (!checkExistAccessTokenFile("token.json") || access_token == "" || refresh_token == "") {

		//Exchange auth code for access token
		if (getAccessTokenFromAuthorizationCode(client_id, client_secret, std::string(authorization_code), redirect_uri, access_token, refresh_token)) {
			// Save access token to file
			saveAccessTokenToFile("token.json", access_token, refresh_token);
			return access_token;
		}
		else {
			return "";
		}
	}

	// Check if the access token is valid
	else {
		if (!isAccessTokenValid(access_token)) {
			std::cout << "Access token is invalid or expired. Refreshing token...\n";
			if (refreshAccessToken(client_id, client_secret, refresh_token, new_access_token)) {
				access_token = new_access_token;  // Update the access token
				saveAccessTokenToFile("token.json", access_token, refresh_token);
				return access_token;
			}
			else {
				return "";
			}
		}
		return access_token;
	}
}

std::vector<std::vector<std::string>> IP_tasks;
std::string access, logHistory;


// Append text to the output window (wide string)
void AppendText(HWND hOutput, const std::wstring& newText) {
	int length = GetWindowTextLengthW(hOutput);
	std::wstring currentText(length, L'\0');
	GetWindowTextW(hOutput, &currentText[0], length + 1);
	currentText += newText + L"\r\n";
	SetWindowTextW(hOutput, currentText.c_str());
	int textLength = GetWindowTextLength(hOutput);
	SendMessage(hOutput, EM_SETSEL, textLength, textLength);
	SendMessage(hOutput, EM_SCROLLCARET, 0, 0);
}

// Append text to the output window (narrow string)
void AppendText(HWND hOutput, const std::string& newText) {
	AppendText(hOutput, StringToWString(newText));
}

// Main processing thread
void mainClient(HWND hOutput) {
	while (true) {
		{
			AppendText(hOutput, L"Checking for new messages...\n");
		}

		std::vector<std::vector<std::string>> IP_tasks = getUnreadMessageContents(access);
		for (const auto& messageContent : IP_tasks) {
			AppendText(hOutput, "New Message Subject: " + messageContent[0]);
			AppendText(hOutput, "New Message Content: " + messageContent[1]);
			AppendText(hOutput, "New Message Sender address: " + messageContent[2]);

			if (messageContent[2] != USER_MAIL) continue;

			std::string ipaddr = messageContent[0];
			struct addrinfo hints = {}, * result = nullptr;

			hints.ai_family = AF_UNSPEC;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;

			int iResult = getaddrinfo(ipaddr.c_str(), DEFAULT_PORT, &hints, &result);
			if (iResult != 0) {
				AppendText(hOutput, "getaddrinfo failed with error: " + std::to_string(iResult));
				WSACleanup();
				continue;
			}

			SOCKET ConnectSocket = INVALID_SOCKET;
			for (auto ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
				ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
				if (ConnectSocket == INVALID_SOCKET) continue;

				if (connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR) {
					closesocket(ConnectSocket);
					ConnectSocket = INVALID_SOCKET;
					continue;
				}
				break;
			}

			freeaddrinfo(result);

			if (ConnectSocket == INVALID_SOCKET) {
				AppendText(hOutput, L"Unable to connect to server!\n");
				continue;
			}

			wchar_t sendbuf[DEFAULT_BUFLEN];
			wchar_t recvbuf[DEFAULT_BUFLEN];

			wcscpy_s(sendbuf, StringToWString(messageContent[1]).c_str());
			if (wcscmp(sendbuf, L"TURNONCAMERA") >= 0) {
				time_t now = time(0);
				wchar_t filename[100];
				swprintf(filename, 100, L"webcam_%lld.avi", (long long)now);
				wcscpy(sendbuf, (L"TURNONCAMERA " + std::wstring(filename)).c_str());
			}

			AppendText(hOutput, L"Client: " + std::wstring(sendbuf));

			iResult = send(ConnectSocket, reinterpret_cast<const char*>(sendbuf), DEFAULT_BUFLEN, 0);
			if (iResult > 0) {

				TASK t = request2TASK(sendbuf);
				std::string sender_email = USER_MAIL;
				std::string recipient_email = messageContent[2];
				std::string subject = "Respone from server " + messageContent[0];
				if (wcscmp(t.TaskName, L"END") == 0) {
					closesocket(ConnectSocket);
					continue;
				}
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
							if (sendEmailViaGmailAPI(access, sender_email, recipient_email, subject, "Failed to get the file from server")) {
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
								if (sendEmailWithAttachment(access, sender_email, recipient_email, subject, body, file_path, file_name)) {
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
							if (sendEmailViaGmailAPI(access, sender_email, recipient_email, subject, "Done")) {
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
		}

		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static HWND hAuthCode, hButton, hButtonLink, hButtonMail, hOutput, hUserMail, hSubmitMail;

	switch (uMsg) {
	case WM_CTLCOLORBTN: {
		HDC hdcButton = (HDC)wParam;
		SetBkColor(hdcButton, RGB(255, 0, 0)); // Chỉnh màu nền là đỏ
		SetTextColor(hdcButton, RGB(255, 255, 255)); // Chỉnh màu chữ là trắng
		return (LRESULT)GetStockObject(DC_BRUSH); // Sử dụng brush có sẵn
	}
	case WM_CREATE:
		CreateWindow(L"STATIC", L"AuthCode:",
			WS_VISIBLE | WS_CHILD,
			20, 20, 80, 20,
			hwnd, NULL, NULL, NULL);
		hAuthCode = CreateWindow(L"EDIT", L"",
			WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
			90, 20, 300, 20,
			hwnd, (HMENU)ID_AUTHCODE, NULL, NULL);

		// Create a button to generate the token
		hButton = CreateWindow(L"BUTTON", L"Generate token",
			WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
			600, 18, 120, 25,
			hwnd, (HMENU)ID_BUTTON, NULL, NULL);
		// Create a button to generate the auth link
		hButtonLink = CreateWindow(L"BUTTON", L"Generate link",
			WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
			400, 18, 120, 25,
			hwnd, (HMENU)ID_BUTTONLINK, NULL, NULL);
		// Create a button to generate the mail
		hButtonMail = CreateWindow(L"BUTTON", L"Generate mail",
			WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
			600, 100, 120, 25,
			hwnd, (HMENU)ID_BUTTONMAIL, NULL, NULL);

		// Create a text area for output
		hOutput = CreateWindowA("EDIT", "",
			WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
			20, 60, 470, 200,
			hwnd, (HMENU)ID_OUTPUT, NULL, NULL);
		CreateWindow(L"STATIC", L"Mail address",
			WS_VISIBLE | WS_CHILD,
			20, 600, 50, 20,
			hwnd, NULL, NULL, NULL);
		hSubmitMail = CreateWindow(L"BUTTON", L"Change mail",
			WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
			370, 600, 120, 25,
			hwnd, (HMENU)ID_SUBMITMAIL, NULL, NULL);
		hUserMail = CreateWindow(L"EDIT", L"",
			WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
			70, 600, 300, 20,
			hwnd, (HMENU)ID_USERMAIL, NULL, NULL);
		SetWindowTextA(hUserMail, USER_MAIL.c_str());
		break;

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		// Tạo một bàn chải màu vàng nhạt
		HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 204));
		FillRect(hdc, &ps.rcPaint, hBrush);

		// Giải phóng bàn chải sau khi sử dụng
		DeleteObject(hBrush);

		EndPaint(hwnd, &ps);
	}
	
	case WM_COMMAND:
		if (LOWORD(wParam) == ID_BUTTONLINK) {

			std::string auth_uri, client_id, client_secret;
			std::string link = Authlink(client_id, client_secret, auth_uri);
			AppendText(hOutput,link + "\n");
		}
		if (LOWORD(wParam) == ID_BUTTON) {
			char* rowsText = new char[1024];
			GetWindowTextA(hAuthCode, rowsText, 1024);
			access = getAccessToken(rowsText);
			AppendText(hOutput, access + "\n");
			int textLength = GetWindowTextLength(hOutput);
			SendMessage(hOutput, EM_SETSEL, textLength, textLength);
			SendMessage(hOutput, EM_SCROLLCARET, 0, 0);
		}
		if (LOWORD(wParam) == ID_BUTTONMAIL) {
			std::thread(mainClient, hOutput).detach();
		}
		if (LOWORD(wParam) == ID_SUBMITMAIL) {
			char* rowsText = new char[1024];
			GetWindowTextA(hUserMail, rowsText, 1024);
			USER_MAIL = rowsText;
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	return 0;
}

// Main entry point
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
	const wchar_t CLASS_NAME[] = L"Client";

	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(
		0, CLASS_NAME, L"Client Application",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
		NULL, NULL, hInstance, NULL
	);

	if (hwnd == NULL) {
		return 0;
	}

	ShowWindow(hwnd, nCmdShow);

	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
