// proj1.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <curl/curl.h>
#include <nlohmann/json.hpp>

// TODO: Reference additional headers your program requires here.
#include <iostream>
#include <sstream>
#include <fstream>
#include <thread>
#include <chrono>

// Struct to handle memory for CURL responses
struct memory {
	char* response;
	size_t size;
};

bool isAccessTokenValid(const std::string& access_token);
bool getAccessTokenFromAuthorizationCode(
	const std::string& client_id,
	const std::string& client_secret,
	const std::string& authorization_code,
	const std::string& redirect_uri,
	std::string& access_token,
	std::string& refresh_token
);
void saveAccessTokenToFile(std::string token_file, const std::string& access_token, const std::string& refresh_token);
bool refreshAccessToken(
	const std::string& client_id,
	const std::string& client_secret,
	const std::string& refresh_token,
	std::string& new_access_token
);


bool checkExistAccessTokenFile(const std::string& name);
void getCredentials(std::string credFilePath, std::string& client_id, std::string& client_secret, std::string& auth_uri);
std::vector<std::vector<std::string>> getUnreadMessageContents(const std::string& access_token);
void readToken(const std::string token_file, std::string& access_token, std::string& refresh_token);
bool sendEmailWithAttachment(
	const std::string& access_token,
	const std::string& sender_email,
	const std::string& recipient_email,
	const std::string& subject,
	const std::string& body,
	const std::string& file_path,
	const std::string& file_name
);
bool sendEmailViaGmailAPI(
	const std::string& access_token,
	const std::string& sender_email,
	const std::string& recipient_email,
	const std::string& subject,
	const std::string& body
);
std::wstring FileName(const std::wstring&);
std::string FileName(const std::string&);