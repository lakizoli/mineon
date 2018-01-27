#pragma once

#include <curl/curl.h>
#include "Json.hpp"
#include "Statistic.hpp"

class CurlClient {
	Statistic& mStatistic;
	CURL* mCurl;
	curl_socket_t mSocket;
	std::vector<char> mCurlErrorBuffer;
	std::vector<uint8_t> mSocketBuffer;

	static int32_t SockOptKeepaliveCallback (void* userdata, curl_socket_t fd, curlsocktype purpose);
	static curl_socket_t OpenSocketGrabCallback (void* clientp, curlsocktype purpose, struct curl_sockaddr *addr);
	static bool SocketFull (curl_socket_t socket, int32_t timeout);
	static bool SendLine (CURL* curl, curl_socket_t socket, std::string str);
	static std::string ReceiveLine (CURL* curl, curl_socket_t socket, std::vector<uint8_t>& buffer);

public:
	explicit CurlClient (Statistic& statistic);

	bool Connect (const std::string& url);
	void Disconnect ();
	bool IsConnected () const {
		return mCurl != nullptr;
	}

	std::shared_ptr<JSONObject> CallRPC (std::shared_ptr<JSONObject> req);
};
