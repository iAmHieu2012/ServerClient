#include "Tasks.h"

int64_t GetFileSize(const wchar_t* fileName) {
	// no idea how to get filesizes > 2.1 GB in a C++ kind-of way.
	// I will cheat and use Microsoft's C-style file API
	FILE* f;
	if (_wfopen_s(&f, fileName, L"rb") != 0) {
		return -1;
	}
	_fseeki64(f, 0, SEEK_END);
	const int64_t len = _ftelli64(f);
	fclose(f);
	return len;
}
//
// Sends a file
// returns size of file if success
// returns -1 if file couldn't be opened for input
// returns -2 if couldn't send file length properly
// returns -3 if file couldn't be sent properly
//
int64_t SendFile(SOCKET s, const wchar_t* fileName, int chunkSize) {

	const int64_t fileSize = GetFileSize(fileName);
	if (fileSize < 0) { return -1; }

	std::ifstream file;
	file.open(fileName, std::ifstream::binary);
	if (file.fail()) { return -1; }

	if (send(s, reinterpret_cast<const char*>(&fileSize),
		sizeof(fileSize), 0) != sizeof(fileSize)) {
		return -2;
	}

	char* buffer = new char[chunkSize];
	bool errored = false;
	int64_t i = fileSize;
	int64_t bufferSize = 0;
	int64_t iResult = 0;
	while (i != 0) {
		bufferSize = __min(i, chunkSize);
		if (!file.read(buffer, bufferSize)) { errored = true; break; }
		iResult = send(s, buffer, bufferSize, 0);
		if (iResult < 0) { errored = true; break; }
		i -= iResult;
	}
	delete[] buffer;

	file.close();

	return errored ? -3 : fileSize;
}

//
// Receives a file
// returns size of file if success
// returns -1 if file couldn't be opened for output
// returns -2 if couldn't receive file length properly
// returns -3 if couldn't receive file properly
//
int64_t RecvFile(SOCKET s, const wchar_t* fileName, int chunkSize) {
	std::ofstream file;
	file.open(fileName, std::ofstream::binary);
	if (file.fail()) { return -1; }

	int64_t fileSize;
	if (recv(s, reinterpret_cast<char*>(&fileSize),
		sizeof(fileSize), 0) != sizeof(fileSize)) {
		return -2;
	}

	char* buffer = new char[chunkSize];
	bool errored = false;
	int64_t i = fileSize;
	int64_t iResult = 0;
	int64_t bufferSize = 0;
	while (i != 0) {
		bufferSize = __min(chunkSize, i);
		iResult = recv(s, buffer, bufferSize, 0);
		if ((iResult < 0) || !file.write(buffer, iResult)) { errored = true; break; }
		i -= iResult;
	}
	delete[] buffer;

	file.close();

	return errored ? -3 : fileSize;
}