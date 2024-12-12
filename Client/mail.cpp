#include "Mail.h"

// Callback function to handle data received by CURL
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}
// Function to verify if the access token is valid
bool isAccessTokenValid(const std::string& access_token) {
	CURL* curl;
	CURLcode res;
	std::string readBuffer;

	// Initialize CURL
	curl = curl_easy_init();
	if (curl) {
		// Set the URL to fetch user profile (for example)
		std::string url = "https://www.googleapis.com/oauth2/v3/tokeninfo?access_token=" + access_token;

		// Set options
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

		// Perform the request
		res = curl_easy_perform(curl);

		// Check if the request was successful
		if (res == CURLE_OK) {
			try {
				// Parse the response JSON using nlohmann::json
				nlohmann::json jsonResponse = nlohmann::json::parse(readBuffer);
				if (jsonResponse.contains("error_description")) {
					std::cerr << "Error: " << jsonResponse["error_description"] << std::endl;
					curl_easy_cleanup(curl);
					return false;  // Token is invalid or expired
				}
				else {
					std::cout << "Token is valid.\n";
					curl_easy_cleanup(curl);
					return true;  // Token is valid
				}
			}
			catch (const std::exception& e) {
				std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
			}
		}
		else {
			std::cerr << "Failed to verify access token: " << curl_easy_strerror(res) << std::endl;
		}

		curl_easy_cleanup(curl);
	}

	std::cerr << "Token is invalid or expired.\n";
	return false;  // Token is invalid or expired
}

// Function to read the access token and refresh token
void readToken(const std::string token_file, std::string& access_token, std::string& refresh_token) {
	std::fstream f(token_file, std::ios::in);
	if (!f.fail()) {
		nlohmann::json tokens = nlohmann::json::parse(f);
		if (tokens.contains("access_token")) {
			access_token = tokens["access_token"];
		}
		else access_token = "";
		if (tokens.contains("refresh_token")) {
			refresh_token = tokens["refresh_token"];
		}
		else refresh_token = "";
	}
	f.close();
}

// Function to send an authorization code for retrieving an access token and refresh token
bool getAccessTokenFromAuthorizationCode(
	const std::string& client_id,
	const std::string& client_secret,
	const std::string& authorization_code,
	const std::string& redirect_uri,
	std::string& access_token,
	std::string& refresh_token
) {
	CURL* curl;
	CURLcode res;
	std::string readBuffer;

	// Prepare POST data
	std::string post_data = "code=" + authorization_code +
		"&client_id=" + client_id +
		"&client_secret=" + client_secret +
		"&redirect_uri=" + redirect_uri +
		"&grant_type=authorization_code";

	curl = curl_easy_init();
	if (curl) {
		// Set the URL and POST data for Google's OAuth token endpoint
		curl_easy_setopt(curl, CURLOPT_URL, "https://oauth2.googleapis.com/token");
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());

		// Capture the response
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

		// Perform the request
		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
			curl_easy_cleanup(curl);
			return false;
		}

		// Parse the response as JSON
		nlohmann::json jsonResponse = nlohmann::json::parse(readBuffer);

		// Check if access token and refresh token exist
		if (jsonResponse.contains("access_token") && jsonResponse.contains("refresh_token")) {
			access_token = jsonResponse["access_token"];
			refresh_token = jsonResponse["refresh_token"];
			curl_easy_cleanup(curl);
			return true;
		}
		else {
			std::cerr << "Error: " << jsonResponse.dump() << std::endl;
			curl_easy_cleanup(curl);
			return false;
		}
	}
	return false;
}

// Function to save the access token to a file
void saveAccessTokenToFile(std::string token_file, const std::string& access_token, const std::string& refresh_token) {
	std::ofstream f(token_file);
	if (f.is_open()) {
		nlohmann::json tokenContent;
		tokenContent["access_token"] = access_token;
		tokenContent["refresh_token"] = refresh_token;
		f << tokenContent;
		f.close();
		std::cout << "Access token saved to file" << std::endl;
	}
	else {
		std::cerr << "Unable to open file to save access token." << std::endl;
	}
}

// Function to refresh the access token
bool refreshAccessToken(
	const std::string& client_id,
	const std::string& client_secret,
	const std::string& refresh_token,
	std::string& new_access_token
) {
	CURL* curl = curl_easy_init();
	if (!curl) return false;

	std::string readBuffer;
	std::string post_data = "client_id=" + client_id +
		"&client_secret=" + client_secret +
		"&refresh_token=" + refresh_token +
		"&grant_type=refresh_token";

	curl_easy_setopt(curl, CURLOPT_URL, "https://oauth2.googleapis.com/token");
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	if (res != CURLE_OK) return false;

	nlohmann::json jsonResponse = nlohmann::json::parse(readBuffer);
	if (jsonResponse.contains("access_token")) {
		new_access_token = jsonResponse["access_token"];
		return true;
	}
	else {
		std::cerr << "Error: " << jsonResponse.dump() << std::endl;
		return false;
	}
}

// Function to encode data in Base64
std::string base64Encode(const std::string& data) {
	static const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string encoded;
	size_t i = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	for (auto it = data.begin(); it != data.end(); ++it) {
		char_array_3[i++] = *it;
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for (i = 0; i < 4; i++)
				encoded += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i) {
		for (size_t j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (size_t j = 0; j < i + 1; j++)
			encoded += base64_chars[char_array_4[j]];

		while (encoded.size() % 4)
			encoded += '=';
	}

	return encoded;
}
// Function to decode Base64 URL-encoded content
std::string base64Decode(const std::string& input) {
	std::string output;
	std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

	// Convert Base64 URL encoding to regular Base64 encoding
	std::string base64_input = input;
	std::replace(base64_input.begin(), base64_input.end(), '-', '+');
	std::replace(base64_input.begin(), base64_input.end(), '_', '/');

	// Padding with '=' to make the input length a multiple of 4
	while (base64_input.length() % 4) {
		base64_input += '=';
	}

	// Decode Base64
	for (size_t i = 0; i < base64_input.size(); i += 4) {
		uint32_t temp = (base64_chars.find(base64_input[i]) << 18) +
			(base64_chars.find(base64_input[i + 1]) << 12) +
			(base64_chars.find(base64_input[i + 2]) << 6) +
			base64_chars.find(base64_input[i + 3]);
		output.push_back((temp >> 16) & 0xFF);
		if (base64_input[i + 2] != '=') output.push_back((temp >> 8) & 0xFF);
		if (base64_input[i + 3] != '=') output.push_back(temp & 0xFF);
	}

	return output;
}

// Function to get details of a message by ID
void getMessageDetails(const std::string& access_token, const std::string& message_id, std::string& subject, std::string& content) {
	CURL* curl;
	CURLcode res;
	std::string readBuffer;

	curl = curl_easy_init();
	if (curl) {
		// Set the URL for fetching a specific message by ID
		std::string url = "https://gmail.googleapis.com/gmail/v1/users/me/messages/" + message_id;
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

		// Set the authorization header
		struct curl_slist* headers = NULL;
		headers = curl_slist_append(headers, ("Authorization: Bearer " + access_token).c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		// Capture the response
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

		// Perform the request
		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
		}
		else {
			// Parse the response as JSON
			nlohmann::json messageResponse = nlohmann::json::parse(readBuffer);

			// Extract subject from headers
			for (const auto& header : messageResponse["payload"]["headers"]) {
				if (header["name"] == "Subject") {
					subject = header["value"];
				}
			}

			// Get message body (if present)
			if (messageResponse["payload"].contains("parts")) {
				for (const auto& part : messageResponse["payload"]["parts"]) {
					if (part["mimeType"] == "text/plain") {
						content = part["body"]["data"]; // Base64url encoded content
						content = base64Decode(content); // Decode the Base64 content
						break; // Exit after the first plain text part
					}
				}
			}
		}

		// Cleanup
		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
	}
}


void markAsRead(const std::string& access_token, const std::string& message_id) {
	CURL* curl;
	CURLcode res;
	std::string readBuffer;

	// Initialize curl
	curl = curl_easy_init();
	if (curl) {
		// Set the URL for the API endpoint
		std::string url = "https://gmail.googleapis.com/gmail/v1/users/me/messages/" + message_id + "/modify";

		// Prepare the JSON payload
		std::string jsonPayload = R"({"removeLabelIds": ["UNREAD"]})";

		// Set curl options
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());

		// Set headers
		struct curl_slist* headers = NULL;
		headers = curl_slist_append(headers, ("Authorization: Bearer " + access_token).c_str());
		headers = curl_slist_append(headers, "Content-Type: application/json");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		// Capture the response
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

		// Set timeout
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // 10 seconds timeout

		//// Enable verbose output for debugging
		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		// Perform the request
		res = curl_easy_perform(curl);

		// Check for errors
		if (res != CURLE_OK) {
			std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
		}
		else {
			// Check HTTP response code
			long response_code;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
			if (response_code == 200) {
				std::cout << "Message marked as read successfully." << std::endl;
			}
			else {
				std::cerr << "Failed to mark as read. HTTP response code: " << response_code << std::endl;
				std::cerr << "Response content: " << readBuffer << std::endl; // Log response for debugging
			}
		}

		// Clean up
		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
	}
}

// Function to get the content of unread Gmail messages
std::vector<std::vector<std::string>> getUnreadMessageContents(const std::string& access_token) {
	CURL* curl;
	CURLcode res;
	std::string readBuffer;
	std::vector<std::string> messageContents;  // To store the content of new messages
	std::vector<std::vector<std::string>> IP_tasks;
	curl = curl_easy_init();
	if (curl) {
		// Set the URL for fetching unread Gmail messages
		curl_easy_setopt(curl, CURLOPT_URL, "https://gmail.googleapis.com/gmail/v1/users/me/messages?labelIds=UNREAD");

		// Set the authorization header
		struct curl_slist* headers = NULL;
		headers = curl_slist_append(headers, ("Authorization: Bearer " + access_token).c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		// Capture the response
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

		// Perform the request
		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
		}
		else {
			// Parse the response as JSON
			nlohmann::json jsonResponse = nlohmann::json::parse(readBuffer);

			// Process each unread message and get the content
			if (jsonResponse.contains("messages")) {
				for (const auto& message : jsonResponse["messages"]) {
					std::string message_id = message["id"];
					std::string content, subject;

					// Get message details (content)
					getMessageDetails(access_token, message_id, subject, content);
					if (subject.length() >= 2 && content.length() >= 2) {
						content[content.length() - 2] = '\0';
						content[content.length() - 1] = '\0';
					}
					// Store the content of the message
					messageContents.push_back(subject);
					messageContents.push_back(content);
					IP_tasks.push_back(messageContents);
					messageContents.clear();

					// Mark the message as read
					markAsRead(access_token, message_id);
				}
			}
			else {
				std::cout << "No unread messages found." << std::endl;
			}
		}

		// Cleanup
		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
	}

	return IP_tasks;
}

void getCredentials(std::string credFilePath, std::string& client_id, std::string& client_secret, std::string& auth_uri) {
	std::fstream f(credFilePath, std::ios::in);
	if (!f.fail()) {
		nlohmann::json creds = nlohmann::json::parse(f);
		client_id = creds["installed"]["client_id"];
		client_secret = creds["installed"]["client_secret"];
		auth_uri = creds["installed"]["auth_uri"];
	}
	f.close();
}
// Function to send an email using Gmail API
bool sendEmailViaGmailAPI(
	const std::string& access_token,
	const std::string& sender_email,
	const std::string& recipient_email,
	const std::string& subject,
	const std::string& body
) {
	CURL* curl = curl_easy_init();
	if (!curl) {
		std::cerr << "Failed to initialize CURL" << std::endl;
		return false;
	}

	// Construct the raw email content
	std::ostringstream email_stream;
	email_stream << "From: " << sender_email << "\r\n"
		<< "To: " << recipient_email << "\r\n"
		<< "Subject: " << subject << "\r\n"
		<< "\r\n"
		<< body;

	std::string raw_email = email_stream.str();
	std::string encoded_email = base64Encode(raw_email);

	// Prepare JSON payload for Gmail API
	nlohmann::json payload;
	payload["raw"] = encoded_email;

	std::string payload_str = payload.dump();

	// Set CURL options
	curl_easy_setopt(curl, CURLOPT_URL, "https://gmail.googleapis.com/gmail/v1/users/me/messages/send");
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_str.c_str());
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload_str.size());
	curl_easy_setopt(curl, CURLOPT_POST, 1L);

	// Set authorization header
	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, ("Authorization: Bearer " + access_token).c_str());
	headers = curl_slist_append(headers, "Content-Type: application/json");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	// Perform the request
	CURLcode res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
		return false;
	}

	// Clean up
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	std::cout << "Email sent successfully!" << std::endl;
	return true;
}
// Function to load file contents
std::string loadFile(const std::string& file_path) {
	std::ifstream file(file_path, std::ios::binary | std::ios::ate);
	if (!file) {
		throw std::runtime_error("Failed to open file: " + file_path);
	}
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(size);
	if (!file.read(buffer.data(), size)) {
		throw std::runtime_error("Failed to read file: " + file_path);
	}

	return std::string(buffer.begin(), buffer.end());
}
std::wstring FileName(const std::wstring& str)
{
	size_t found = str.find_last_of(L"/\\");
	std::wstring path = str.substr(found + 1); // check that is OK
	return path;
}
std::string FileName(const std::string& str)
{
	size_t found = str.find_last_of("/\\");
	std::string path = str.substr(found + 1); // check that is OK
	return path;
}
// Function to send email via Gmail API with attachment
bool sendEmailWithAttachment(
	const std::string& access_token,
	const std::string& sender_email,
	const std::string& recipient_email,
	const std::string& subject,
	const std::string& body,
	const std::string& file_path,
	const std::string& file_name
) {
	CURL* curl = curl_easy_init();
	if (!curl) {
		std::cerr << "Failed to initialize CURL" << std::endl;
		return false;
	}

	// Load and encode file as Base64
	std::string file_content = loadFile(file_path);
	std::string encoded_file = base64Encode(file_content);

	// Construct MIME message
	std::ostringstream mime_stream;
	mime_stream << "From: " << sender_email << "\r\n"
		<< "To: " << recipient_email << "\r\n"
		<< "Subject: " << subject << "\r\n"
		<< "Content-Type: multipart/mixed; boundary=\"boundary\"\r\n\r\n"
		<< "--boundary\r\n"
		<< "Content-Type: text/plain; charset=\"UTF-8\"\r\n"
		<< "Content-Transfer-Encoding: 7bit\r\n\r\n"
		<< body << "\r\n\r\n"
		<< "--boundary\r\n"
		<< "Content-Type: application/octet-stream; name=\"" << file_name << "\"\r\n"
		<< "Content-Transfer-Encoding: base64\r\n"
		<< "Content-Disposition: attachment; filename=\"" << file_name << "\"\r\n\r\n"
		<< encoded_file << "\r\n"
		<< "--boundary--";


	std::string raw_email = mime_stream.str();
	std::string encoded_email = base64Encode(raw_email);

	// Prepare JSON payload for Gmail API
	nlohmann::json payload;
	payload["raw"] = encoded_email;

	std::string payload_str = payload.dump();

	// Set CURL options
	curl_easy_setopt(curl, CURLOPT_URL, "https://gmail.googleapis.com/gmail/v1/users/me/messages/send");
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_str.c_str());
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload_str.size());
	curl_easy_setopt(curl, CURLOPT_POST, 1L);

	// Set authorization header
	struct curl_slist* headers = nullptr;
	headers = curl_slist_append(headers, ("Authorization: Bearer " + access_token).c_str());
	headers = curl_slist_append(headers, "Content-Type: application/json");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	// Perform the request
	CURLcode res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
		return false;
	}

	// Clean up
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	std::cout << "Email sent successfully with attachment!" << std::endl;
	return true;
}

bool checkExistAccessTokenFile(const std::string& name) {
	std::ifstream f(name);
	return f.good();
}