#include "stdafx.h"
#include "Stratum.hpp"
#include "Json.hpp"

#include <Winsock2.h>
#include <Mstcpip.h>

static int32_t sockopt_keepalive_cb (void* userdata, curl_socket_t fd, curlsocktype purpose) {
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

static curl_socket_t opensocket_grab_cb (void* clientp, curlsocktype purpose, struct curl_sockaddr *addr) {
	curl_socket_t* sock = (curl_socket_t*) clientp;
	*sock = socket (addr->family, addr->socktype, addr->protocol);
	return *sock;
}

static bool send_line (CURL* curl, curl_socket_t socket, std::string str) {
	size_t len = str.size ();
	size_t sent = 0;

	str.push_back ('\n');

	while (len > 0) {
		struct timeval timeout {0, 0};
		fd_set wd;

		FD_ZERO(&wd);
		FD_SET(socket, &wd);
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

static bool socket_full (curl_socket_t socket, int32_t timeout) {
	struct timeval tv { timeout, 0 };

	fd_set rd;
	FD_ZERO (&rd);
	FD_SET (socket, &rd);
	return select (socket + 1, &rd, nullptr, nullptr, &tv) > 0;
}

static std::string recv_line (CURL* curl, curl_socket_t socket, std::vector<uint8_t>& buffer) {
	auto it = std::find (buffer.begin (), buffer.end (), '\n');
	if (it == buffer.end ()) {
		const std::chrono::system_clock::time_point rstart = std::chrono::system_clock::now ();
		if (!socket_full (socket, 60)) {
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
				if (rc != CURLE_AGAIN || !socket_full(socket, 1)) {
					ret = false;
					break;
				}
			} else {
				std::copy (recv.begin (), recv.end (), std::back_inserter (buffer));
			}

			it = std::find (buffer.begin (), buffer.end (), '\n');
		} while (std::chrono::system_clock::now () - rstart < std::chrono::seconds (60) && it == buffer.end ());

		if (!ret) {
			//applog(LOG_ERR, "stratum_recv_line failed");
			return std::string ();
		}
	}

	std::string res (buffer.begin (), it);
	buffer.erase (buffer.begin (), it);
	return res;
}

static std::shared_ptr<JSONObject> CallRPC (CURL* curl, curl_socket_t socket, std::vector<uint8_t>& buffer, std::shared_ptr<JSONObject> req) {
	//Get request
	if (req == nullptr) {
		return nullptr;
	}

	std::string data = req->ToString ();
	data.push_back ('\n');

	//Execute the JSON RPC call
	if (!send_line (curl, socket, data)) {
		return nullptr;
	}

	if (!socket_full (socket, 30)) { //Timeout in socket
		return nullptr;
	}

	std::string rline = recv_line (curl, socket, buffer);
	if (rline.empty ()) {
		return nullptr;
	}

	return JSONObject::Parse (rline);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool Stratum::Connect (const std::string& url) {
	//Comose stratum url
	size_t posSeparator = url.find (':');
	if (posSeparator == std::string::npos) {
		mStatistic.Error ("Stratum: Malformed url given! No scheme found! url: " + url);
		return false;
	}

	if (url.length () < posSeparator + 1) {
		mStatistic.Error ("Stratum: Malformed url given! Length error! url: " + url);
		return false;
	}

	mUrl = "http" + url.substr (posSeparator);

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
	curl_easy_setopt (mCurl, CURLOPT_URL, mUrl.c_str ());
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
	curl_easy_setopt (mCurl, CURLOPT_SOCKOPTFUNCTION, sockopt_keepalive_cb);
	curl_easy_setopt (mCurl, CURLOPT_OPENSOCKETFUNCTION, opensocket_grab_cb);
	curl_easy_setopt (mCurl, CURLOPT_OPENSOCKETDATA, &mSocket);
	curl_easy_setopt (mCurl, CURLOPT_CONNECT_ONLY, 1);

	CURLcode rc = curl_easy_perform (mCurl);
	if (rc != CURLE_OK) {
		mStatistic.Error ("Stratum: CURL connection failed! error: " + std::string (&mCurlErrorBuffer[0]));
		return false;
	}

	//TODO: ...
	return true;
}

void Stratum::Disconnect () {
	if (mCurl) {
		curl_easy_cleanup (mCurl);
		mCurl = nullptr;
		mSocket = 0;
	}
}

bool Stratum::Subscribe () {
	bool succeeded = false;

	uint32_t tryCount = 2;
	while (!succeeded && tryCount--) {
		//Compose request
		std::shared_ptr<JSONObject> req = JSONObject::Create ();
		req->Add ("id", 1);
		req->Add ("method", "mining.subscribe");

		std::shared_ptr<JSONArray> params = JSONArray::Create ();
		if (tryCount == 1) { //This is the first try
			params->Add (mConfig->GetUserAgent ());
			if (!mSessionID.empty ()) {
				params->Add (mSessionID);
			}
		}

		req->Add ("params", params);

		//Execute call
		std::shared_ptr<JSONObject> resp = CallRPC (mCurl, mSocket, mSocketBuffer, req);
		if (resp == nullptr) {
			if (tryCount > 0) { //Retry
				continue;
			}

			//Error
			return false;
		}

		//Parse response {"error": null, "id": 1, "result": [["mining.notify", "ae6812eb4cd7735a302a8a9dd95cf71f"], "f8004b0a", 4]}
		if (!resp->HasArray ("result") || (resp->HasObject ("error") && !resp->GetObj ("error")->IsEmpty ())) {
			if (tryCount > 0) { //Retry
				continue;
			}

			//Error
			return false;
		}

		std::shared_ptr<JSONArray> result = resp->GetArray ("result");
		if (result->GetCount () <= 0) {
			if (tryCount > 0) { //Retry
				continue;
			}

			//Error
			return false;
		}

		mExtraNonce2Size = 0;
		mExtraNonce.clear ();
		mSessionID.clear ();
		for (int32_t i = 0, iEnd = result->GetCount(); i < iEnd; ++i) {
			if (result->HasArrayAtIndex(i)) { //Notify array
				std::shared_ptr<JSONArray> arr = result->GetArrayAtIndex (i);
				if (arr->GetCount () >= 2 && arr->HasStringAtIndex (0) && arr->GetStringAtIndex (0) == "mining.notify") {
					mSessionID = arr->GetStringAtIndex (1);
				}
			} else if (result->HasStringAtIndex (i)) {
				std::string extraNonce = result->GetStringAtIndex (i);
				for (size_t i = 0; i < extraNonce.length (); i += 2) {
					mExtraNonce.push_back ((uint8_t) stoul (extraNonce.substr (i, 2), 0, 16));
				}
			} else if (result->HasUInt32AtIndex (i)) {
				mExtraNonce2Size = result->GetUInt32AtIndex (i);
			}
		}

		if (mSessionID.empty () || mExtraNonce.empty () || mExtraNonce2Size <= 0) {
			if (tryCount > 0) { //Retry
				continue;
			}

			//Error
			return false;
		}

		succeeded = true;
	}

	return succeeded;
}

bool Stratum::Authorize () {
	//TODO: ...
	return false;
}

Stratum::Stratum (Statistic& statistic, Workshop& workshop, std::shared_ptr<Config> cfg) :
	Network (statistic, workshop, cfg),
	mCurl (nullptr),
	mExtraNonce2Size (0)
{
}

void Stratum::Step () {
	//Connect to the stratum server
	while (!mCurl) {
		if (!Connect (mConfig->GetUrl ()) || !Subscribe () || !Authorize ()) {
			Disconnect ();

			mStatistic.Error ("Stratum: Connection error! Retry after 5 seconds...");
			std::this_thread::sleep_for (std::chrono::seconds (5));
		} else {
			mStatistic.Message ("Stratum: Connected successfully!");
		}
	}






	//mWorkshop.SetNewJob (job);

	//Wait for a little bit before reset
	std::this_thread::sleep_for (std::chrono::seconds (30));
}
