#include "stdafx.h"
#include "Stratum.hpp"
#include "Json.hpp"
#include "CurlClient.hpp"

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

	//Connect client
	return mCurlClient.Connect (mUrl);
}

void Stratum::Disconnect () {
	mCurlClient.Disconnect ();
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
		std::shared_ptr<JSONObject> resp = mCurlClient.CallRPC (req);
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
	mCurlClient (statistic),
	mExtraNonce2Size (0)
{
}

void Stratum::Step () {
	//Connect to the stratum server
	while (!mCurlClient.IsConnected ()) {
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
