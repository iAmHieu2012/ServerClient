#include "Server.h"
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <ctime>
#include <cstdlib>
#include <iomanip>
#include <thread>

#define ID_OUTPUT 104

// Helper function to append text to the output window
void AppendText(HWND hOutput, const std::wstring& newText) {
	//std::lock_guard<std::mutex> lock(logMutex);

	int length = GetWindowTextLengthW(hOutput);
	std::wstring currentText(length, L'\0');
	GetWindowTextW(hOutput, &currentText[0], length + 1);

	currentText += newText + L"\r\n";
	SetWindowTextW(hOutput, currentText.c_str());
	int textLength = GetWindowTextLength(hOutput);
	SendMessage(hOutput, EM_SETSEL, textLength, textLength);
	SendMessage(hOutput, EM_SCROLLCARET, 0, 0);
}

// Network handler (runs in a separate thread)
void mainServer(HWND hOutput) {
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		AppendText(hOutput, L"WSAStartup failed with error: " + std::to_wstring(iResult));
		return;
	}
	AppendText(hOutput, L"WSAStartup initialized successfully.");

	struct addrinfo* result = nullptr;
	struct addrinfo hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		AppendText(hOutput, L"getaddrinfo failed with error: " + std::to_wstring(iResult));
		WSACleanup();
		return;
	}

	SOCKET ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		AppendText(hOutput, L"Socket creation failed with error: " + std::to_wstring(WSAGetLastError()));
		freeaddrinfo(result);
		WSACleanup();
		return;
	}

	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		AppendText(hOutput, L"Bind failed with error: " + std::to_wstring(WSAGetLastError()));
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}
	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		AppendText(hOutput, L"Listen failed with error: " + std::to_wstring(WSAGetLastError()));
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}
	AppendText(hOutput, L"Server listening on port " DEFAULT_PORT);

	while (true) {
		SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			AppendText(hOutput, L"Accept failed with error: " + std::to_wstring(WSAGetLastError()));
			closesocket(ListenSocket);
			WSACleanup();
			return;
		}
		AppendText(hOutput, L"Client connected.");

		wchar_t recvbuf[DEFAULT_BUFLEN] = { 0 };
		iResult = recv(ClientSocket, reinterpret_cast<char*>(recvbuf), DEFAULT_BUFLEN, 0);
		if (iResult > 0) {
			AppendText(hOutput, L"Client: " + std::wstring(recvbuf));

			// Parse the request into a TASK structure
			TASK task = request2TASK(recvbuf);

			// Process the task
			int taskResult = doTasks(ClientSocket, task);
			if (taskResult <= 0) {
				AppendText(hOutput, L"Error while processing task.");
			}
		}
		else if (iResult == 0) {
			AppendText(hOutput, L"Connection closing...");
		}
		else {
			AppendText(hOutput, L"Recv failed with error: " + std::to_wstring(WSAGetLastError()));
		}
		closesocket(ClientSocket);
	}

	closesocket(ListenSocket);
	WSACleanup();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static HWND hOutput;

	switch (uMsg) {
	case WM_CREATE:
		CreateWindow(L"STATIC", L"Status:",
			WS_VISIBLE | WS_CHILD,
			20, 40, 80, 20,
			hwnd, NULL, NULL, NULL);
		// Create a text area for output
		hOutput = CreateWindowA(
			"EDIT", "",
			WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
			20, 60, 470, 200,
			hwnd, (HMENU)ID_OUTPUT, NULL, NULL);

		// Start the network handler thread
		std::thread(mainServer, hOutput).detach();
		break;

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 204));
		FillRect(hdc, &ps.rcPaint, hBrush);

		DeleteObject(hBrush);

		EndPaint(hwnd, &ps);
		break;
	}
	case WM_CTLCOLORBTN: {
		HDC hdcButton = (HDC)wParam;

		SetBkColor(hdcButton, RGB(173, 216, 230));
		SetTextColor(hdcButton, RGB(0, 0, 0));

		static HBRUSH hBrushButton = CreateSolidBrush(RGB(173, 216, 230));
		return (LRESULT)hBrushButton;
	}


	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return 0;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
	const wchar_t CLASS_NAME[] = L"Server";

	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(
		0, CLASS_NAME, L"Server Application",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 600, 360,
		NULL, NULL, hInstance, NULL);

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