#include "stdafx.h"
#include "CurlClient.hpp"

#include <Winsock2.h>
#include <Mstcpip.h>

#define COMMUNICATION_DEBUG

int32_t CurlClient::SockOptKeepaliveCallback (void* userdata, curl_socket_t fd, curlsocktype purpose) {
	int32_t keepalive = 1;
	int32_t tcp_keepcnt = 3;
	int32_t tcp_keepidle = 50;
	int32_t tcp_keepintvl = 50;

#ifndef WIN32
	if (unlikely (setsockopt (fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive,
		sizeof (keepalive))))
		return 1;
#ifdef __linux
	if (unlikely (setsockopt (fd, SOL_TCP, TCP_KEEPCNT,
		&tcp_keepcnt, sizeof (tcp_keepcnt))))
		return 1;
	if (unlikely (setsockopt (fd, SOL_TCP, TCP_KEEPIDLE,
		&tcp_keepidle, sizeof (tcp_keepidle))))
		return 1;
	if (unlikely (setsockopt (fd, SOL_TCP, TCP_KEEPINTVL,
		&tcp_keepintvl, sizeof (tcp_keepintvl))))
		return 1;
#endif /* __linux */
#ifdef __APPLE_CC__
	if (unlikely (setsockopt (fd, IPPROTO_TCP, TCP_KEEPALIVE,
		&tcp_keepintvl, sizeof (tcp_keepintvl))))
		return 1;
#endif /* __APPLE_CC__ */
#else /* WIN32 */
	struct tcp_keepalive vals;

	vals.onoff = 1;
	vals.keepalivetime = tcp_keepidle * 1000;
	vals.keepaliveinterval = tcp_keepintvl * 1000;

	DWORD outputBytes;
	if (WSAIoctl (fd, SIO_KEEPALIVE_VALS, &vals, sizeof (vals), nullptr, 0, &outputBytes, nullptr, nullptr) != 0) {
		return 1;
	}
#endif /* WIN32 */

	return 0;
}

curl_socket_t CurlClient::OpenSocketGrabCallback (void* clientp, curlsocktype purpose, struct curl_sockaddr *addr) {
	curl_socket_t* sock = (curl_socket_t*) clientp;
	*sock = socket (addr->family, addr->socktype, addr->protocol);
	return *sock;
}

bool CurlClient::SocketFull (curl_socket_t socket, int32_t timeout) {
	struct timeval tv { timeout, 0 };

	fd_set rd;
	FD_ZERO (&rd);
	FD_SET (socket, &rd);
	return select (socket + 1, &rd, nullptr, nullptr, &tv) > 0;
}

void CurlClient::AppendStringToDebugFile (const std::string& tag, const std::string& str) {
	std::fstream file ("d:\\work\\messages.dump", std::ios::out | std::ios::binary | std::ios::app);
	if (file) {
		file.write ("====================\n", 21);
		file.write (tag.c_str (), tag.size ());
		file.write ("\n", 1);
		file.write (str.c_str (), str.size ());
	}
}

bool CurlClient::SendLine (CURL* curl, curl_socket_t socket, std::string str) {
#ifdef COMMUNICATION_DEBUG
	AppendStringToDebugFile ("request", str);
#endif //COMMUNICATION_DEBUG

	size_t len = str.size ();
	size_t sent = 0;

	str.push_back ('\n');

	while (len > 0) {
		struct timeval timeout { 0, 0 };
		fd_set wd;

		FD_ZERO (&wd);
		FD_SET (socket, &wd);
		if (select (socket + 1, nullptr, &wd, nullptr, &timeout) < 1) {
			return false;
		}

		size_t n = 0;
		const CURLcode rc = curl_easy_send (curl, (&str[0]) + sent, len, &n);
		if (rc != CURLE_OK) {
			if (rc != CURLE_AGAIN) {
				return false;
			}
			n = 0;
		}
		sent += n;
		len -= n;
	}

	return true;
}

std::string CurlClient::ReceiveLine (CURL* curl, curl_socket_t socket, std::vector<uint8_t>& buffer, int32_t timeout) {
	auto it = std::find (buffer.begin (), buffer.end (), '\n');
	if (it == buffer.end ()) {
		const std::chrono::system_clock::time_point rstart = std::chrono::system_clock::now ();
		if (!SocketFull (socket, timeout)) {
			//applog(LOG_ERR, "stratum_recv_line timed out");
			return std::string ();
		}

		bool ret = true;
		do {
			std::vector<uint8_t> recv (2048);
			size_t n = 0;

			const CURLcode rc = curl_easy_recv (curl, &recv[0], 2048, &n);
			if (rc == CURLE_OK && n == 0) { //Nothing arrived
				ret = false;
				break;
			}

			if (rc != CURLE_OK) {
				if (rc != CURLE_AGAIN || !SocketFull (socket, 1)) {
					ret = false;
					break;
				}
			} else {
				std::copy (recv.begin (), recv.begin () + n, std::back_inserter (buffer));
			}

			it = std::find (buffer.begin (), buffer.end (), '\n');
		} while (std::chrono::system_clock::now () - rstart < std::chrono::seconds (60) && it == buffer.end ());

		if (!ret) {
			//applog(LOG_ERR, "stratum_recv_line failed");
			return std::string ();
		}
	}

	std::string res (buffer.begin (), it);
	buffer.erase (buffer.begin (), ++it); //++it for erase the newline char from end also!

#ifdef COMMUNICATION_DEBUG
	AppendStringToDebugFile ("response", res);
#endif //COMMUNICATION_DEBUG

	return res;
}

CurlClient::CurlClient (Statistic& statistic) :
	mStatistic (statistic),
	mCurl (nullptr)
{
}

bool CurlClient::Connect (const std::string& url) {
	//Init buffers
	mSocketBuffer.clear ();

	mCurlErrorBuffer.resize (256);
	memset (&mCurlErrorBuffer[0], 0, mCurlErrorBuffer.size ());

	//Init curl
	mCurl = curl_easy_init ();
	if (!mCurl) {
		mStatistic.Error ("Stratum: CURL initialization failed!");
		return false;
	}

	//Configure curl
	//if (opt_protocol) {
	//	curl_easy_setopt (curl, CURLOPT_VERBOSE, 1);
	//}
	curl_easy_setopt (mCurl, CURLOPT_URL, url.c_str ());
	//if (opt_cert) {
	//	curl_easy_setopt (curl, CURLOPT_CAINFO, opt_cert);
	//}
	curl_easy_setopt (mCurl, CURLOPT_FRESH_CONNECT, 1);
	curl_easy_setopt (mCurl, CURLOPT_CONNECTTIMEOUT, 30);
	curl_easy_setopt (mCurl, CURLOPT_ERRORBUFFER, &mCurlErrorBuffer[0]);
	curl_easy_setopt (mCurl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt (mCurl, CURLOPT_TCP_NODELAY, 1);
	//if (opt_proxy) {
	//	curl_easy_setopt (curl, CURLOPT_PROXY, opt_proxy);
	//	curl_easy_setopt (curl, CURLOPT_PROXYTYPE, opt_proxy_type);
	//}
	curl_easy_setopt (mCurl, CURLOPT_HTTPPROXYTUNNEL, 1);
	curl_easy_setopt (mCurl, CURLOPT_SOCKOPTFUNCTION, CurlClient::SockOptKeepaliveCallback);
	curl_easy_setopt (mCurl, CURLOPT_OPENSOCKETFUNCTION, CurlClient::OpenSocketGrabCallback);
	curl_easy_setopt (mCurl, CURLOPT_OPENSOCKETDATA, &mSocket);
	curl_easy_setopt (mCurl, CURLOPT_CONNECT_ONLY, 1);

	CURLcode rc = curl_easy_perform (mCurl);
	if (rc != CURLE_OK) {
		mStatistic.Error ("Stratum: CURL connection failed! error: " + std::string (&mCurlErrorBuffer[0]));
		return false;
	}

	return true;
}

void CurlClient::Disconnect () {
	if (mCurl) {
		curl_easy_cleanup (mCurl);
		mCurl = nullptr;
		mSocket = 0;
	}
}

std::shared_ptr<JSONObject> CurlClient::CallJsonRPC (std::shared_ptr<JSONObject> req) {
	//Execute the JSON RPC call
	if (!SendJson (req)) {
		return nullptr;
	}

	if (!SocketFull (mSocket, 30)) { //Timeout in socket
		return nullptr;
	}

	return ReceiveJson ();
}

std::shared_ptr<JSONObject> CurlClient::ReceiveJson (int32_t timeout) {
	std::string rline = ReceiveLine (mCurl, mSocket, mSocketBuffer, timeout);
	if (rline.empty ()) {
		return nullptr;
	}

	return JSONObject::Parse (rline);
}

bool CurlClient::SendJson (std::shared_ptr<JSONObject> json) const {
	//Get data
	if (json == nullptr) {
		return false;
	}

	std::string data = json->ToString ();
	data.push_back ('\n');

	//Send json line to the server
	return SendLine (mCurl, mSocket, data);
}
