#include "stdafx.h"
#include "Stratum.hpp"
#include "Json.hpp"
#include "CurlClient.hpp"
#include "Sha2Utils.hpp"

#define RUN_STRATUM_TEST

void Stratum::RunTest () {
	// Test communication with succeeded result:
	//
	//request:	{"id":1,"method":"mining.subscribe","params":["mineon 1.0.0"]}
	//response:	{"error": null, "id": 1, "result": [["mining.notify", "ae6812eb4cd7735a302a8a9dd95cf71f"], "f8006343", 4]}
	//
	//request:	{"id":2,"method":"mining.authorize","params":["lzgm.2","x"]}
	//response:	{"params": [2048.0], "id": null, "method": "mining.set_difficulty"}
	//
	//notify:	{"params": ["4dcd", "5ee7062d191ee0f3bbbea2babae090b05f8c9530948397caa5402784760a8859", "01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff530310041d062f503253482f047b66725a08", "392f7374726174756d506f6f6c2ffabe6d6db90d513dd69e5fe6fb72f48a813ba3eb6d8d2fc34fb21761390dc21d1d75e61310000000000000000000000001807c814a000000001976a9146fc22a4a9b1e7d83ce8c40d94648105b9b50c02b88ac00000000", [], "00000002", "1b01784b", "5a72667b", true], "id": null, "method": "mining.notify"}
	//result:	{"id":4,"method":"mining.submit","params":["lzgm.2","4dcd","00000000","5a72667b","2bbb1bc7"]}

	//subscribe
	mSessionID = "ae6812eb4cd7735a302a8a9dd95cf71f";
	mExtraNonce = { 0xf8, 0x00, 0x63, 0x43 };
	mExtraNonce2Size = 4;

	//set_difficulty
	mDifficulty = 2048.0;

	//notify
	std::string notify = "{\"params\": [\"4dcd\", \"5ee7062d191ee0f3bbbea2babae090b05f8c9530948397caa5402784760a8859\", \"01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff530310041d062f503253482f047b66725a08\", \"392f7374726174756d506f6f6c2ffabe6d6db90d513dd69e5fe6fb72f48a813ba3eb6d8d2fc34fb21761390dc21d1d75e61310000000000000000000000001807c814a000000001976a9146fc22a4a9b1e7d83ce8c40d94648105b9b50c02b88ac00000000\", [], \"00000002\", \"1b01784b\", \"5a72667b\", true], \"id\": null, \"method\": \"mining.notify\"}";
	std::shared_ptr<JSONObject> json = JSONObject::Parse (notify);
	HandleMethod (json);

	std::this_thread::sleep_for (std::chrono::seconds (3600));
}

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
		//Clear stratum values
		mSessionID.clear ();
		mExtraNonce.clear ();
		mExtraNonce2Size = 0;
		mDifficulty = 0.0;

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
		std::shared_ptr<JSONObject> resp = mCurlClient.CallJsonRPC (req);
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
	//Compose request
	std::shared_ptr<JSONObject> req = JSONObject::Create ();
	req->Add ("id", 2);
	req->Add ("method", "mining.authorize");

	std::shared_ptr<JSONArray> params = JSONArray::Create ();
	params->Add (mConfig->GetUser ());
	params->Add (mConfig->GetPassword ());
	req->Add ("params", params);

	//Execute call
	std::shared_ptr<JSONObject> resp = mCurlClient.CallJsonRPC (req);

	//Handle all response
	while (resp && HandleMethod (resp)) {
		resp = mCurlClient.ReceiveJson ();
	}

	return true;
}

bool Stratum::HandleMethod (std::shared_ptr<JSONObject> json) {
	std::string method = json->GetString ("method");
	if (method.empty ()) {
		return false;
	}

	if (method == "mining.notify") {
		std::shared_ptr<JSONArray> params = json->GetArray ("params");
		if (params && params->GetCount () >= 8) {
			return HandleNotify (params);
		}
		return false;
	} else if (method == "mining.set_difficulty") {
		std::shared_ptr<JSONArray> params = json->GetArray ("params");
		if (params && params->GetCount () >= 1) {
			mDifficulty = params->GetDoubleAtIndex (0);
			return true;
		}
	} else if (method == "client.reconnect") {
		std::shared_ptr<JSONArray> params = json->GetArray ("params");
		if (params && params->GetCount () >= 2) {
			std::string host = params->GetStringAtIndex (0);
			uint32_t port = 0;
			if (params->HasStringAtIndex (1)) {
				port = stoul (params->GetStringAtIndex (1));
			} else if (params->HasUInt32AtIndex (1)) {
				port = params->GetUInt32AtIndex (1);
			}

			if (port <= 0) {
				return false;
			}

			mStatistic.Message ("Stratum: Server requested reconnection to host: " + host + " on port: " + std::to_string (port));
			mConfig->SetHostAndPort (host, port);
			Disconnect ();
			return true;
		}
	} else if (method == "client.get_version") {
		uint32_t id = json->GetUInt32 ("id");
		if (id > 0) {
			std::shared_ptr<JSONObject> req = JSONObject::Create ();
			req->Add ("id", id);
			req->AddNull ("error");
			req->Add ("result", mConfig->GetUserAgent ());
			return mCurlClient.SendJson (req);
		}
	} else if (method == "client.show_message") {
		uint32_t id = json->GetUInt32 ("id");
		std::shared_ptr<JSONArray> params = json->GetArray ("params");
		if (id > 0 && params && params->GetCount () >= 1) {
			std::string msg = params->GetStringAtIndex (0);
			if (!msg.empty ()) {
				mStatistic.Message ("Stratum: Message from server -> '" + msg + "'");
			}

			std::shared_ptr<JSONObject> req = JSONObject::Create ();
			req->Add ("id", id);
			req->AddNull ("error");
			req->Add ("result", true);
			return mCurlClient.SendJson (req);
		}
	}

	return false;
}

bool Stratum::HandleNotify (std::shared_ptr<JSONArray> notifyParams) {
	JobInfo jobInfo;

	//Parse parameters
	jobInfo.jobID = notifyParams->GetStringAtIndex (0);
	if (jobInfo.jobID.empty ()) {
		return false;
	}

	std::string prevHash = notifyParams->GetStringAtIndex (1);
	if (prevHash.length () != 64) {
		return false;
	}

	std::string coinBase1 = notifyParams->GetStringAtIndex (2);
	if (coinBase1.empty ()) {
		return false;
	}

	std::string coinBase2 = notifyParams->GetStringAtIndex (3);
	if (coinBase2.empty ()) {
		return false;
	}

	std::shared_ptr<JSONArray> merkleArray = notifyParams->GetArrayAtIndex (4);
	if (merkleArray == nullptr) {
		return false;
	}

	std::string version = notifyParams->GetStringAtIndex (5);
	if (version.length () != 8) {
		return false;
	}

	std::string nbits = notifyParams->GetStringAtIndex (6);
	if (nbits.length () != 8) {
		return false;
	}

	std::string ntime = notifyParams->GetStringAtIndex (7);
	if (ntime.length () != 8) {
		return false;
	}

	if (!notifyParams->HasBoolAtIndex (8)) {
		return false;
	}

	jobInfo.clean = notifyParams->GetBoolAtIndex (8);

	//Convert merkle tree
	for (uint32_t i = 0, iEnd = merkleArray->GetCount (); i < iEnd; ++i) {
		std::string merkleItemValue = merkleArray->GetStringAtIndex (i);
		if (merkleItemValue.length () != 64) {
			return false;
		}

		std::vector<uint8_t> merkleItem;
		for (size_t j = 0, jEnd = merkleItemValue.length (); j < jEnd; j += 2) {
			merkleItem.push_back ((uint8_t) stoul (merkleItemValue.substr (j, 2), 0, 16));
		}

		jobInfo.merkleTree.push_back (merkleItem);
	}

	//Compose coinbase -> [coinbase1] + [xnonce1] + [xnonce2] + [coinbase2]
	jobInfo.extraNonce2Pos = (uint32_t) (coinBase1.length () / 2 + mExtraNonce.size ());
	for (size_t i = 0, iEnd = coinBase1.length (); i < iEnd; i += 2) {
		jobInfo.coinBase.push_back ((uint8_t) stoul (coinBase1.substr (i, 2), 0, 16));
	}
	std::copy (mExtraNonce.begin (), mExtraNonce.end (), std::back_inserter (jobInfo.coinBase));
	for (size_t i = 0; i < mExtraNonce2Size; ++i) {
		jobInfo.coinBase.push_back (0);
	}
	for (size_t i = 0, iEnd = coinBase2.length (); i < iEnd; i += 2) {
		jobInfo.coinBase.push_back ((uint8_t) stoul (coinBase2.substr (i, 2), 0, 16));
	}

	//Convert prevHash
	for (size_t i = 0, iEnd = prevHash.length (); i < iEnd; i += 2) {
		jobInfo.prevHash.push_back ((uint8_t) stoul (prevHash.substr (i, 2), 0, 16));
	}

	//Convert version
	for (size_t i = 0, iEnd = version.length (); i < iEnd; i += 2) {
		jobInfo.version.push_back ((uint8_t) stoul (version.substr (i, 2), 0, 16));
	}

	//Convert nbits
	for (size_t i = 0, iEnd = nbits.length (); i < iEnd; i += 2) {
		jobInfo.nBits.push_back ((uint8_t) stoul (nbits.substr (i, 2), 0, 16));
	}

	//Convert ntime
	for (size_t i = 0, iEnd = ntime.length (); i < iEnd; i += 2) {
		jobInfo.nTime.push_back ((uint8_t) stoul (ntime.substr (i, 2), 0, 16));
	}

	//Generate job
	GenerateJob (jobInfo);
	return true;
}

void Stratum::SubmitJobResults () {
	std::vector<JobResult> results = mWorkshop.GetJobResults ();

	for (const JobResult& result : results) {
		bool succeeded = false;

		//Compose request
		std::shared_ptr<JSONObject> req = JSONObject::Create ();
		req->Add ("id", 4);
		req->Add ("method", "mining.submit");

		uint32_t value = 0;
		LittleEndianUInt32Encode ((uint8_t*) &value, result.nTime);
		std::string nTime = ToHexString ((const uint8_t*) &value, sizeof (uint32_t));

		value = 0;
		LittleEndianUInt32Encode ((uint8_t*) &value, result.nonce);
		std::string nonce = ToHexString ((const uint8_t*) &value, sizeof (uint32_t));

		std::string xNonce2 = ToHexString (&result.xNonce2[0], result.xNonce2.size ());

		std::shared_ptr<JSONArray> params = JSONArray::Create ();
		params->Add (mConfig->GetUser ());
		params->Add (result.jobID);
		params->Add (xNonce2);
		params->Add (nTime);
		params->Add (nonce);
		req->Add ("params", params);

		//Execute call
		std::shared_ptr<JSONObject> resp = mCurlClient.CallJsonRPC (req);

		//Handle response
		succeeded = resp->HasBool ("result") && resp->GetBool ("result");
		mWorkshop.RemoveSubmittedJobResult (result.jobID);

		if (succeeded) {
			mStatistic.Message ("Stratum: work submitted! jobID: " + result.jobID + " (Yay!!!!)");
		} else { //Failed or rejected
			mStatistic.Error ("Stratum: submitted work rejected! jobID: " + result.jobID + " (Boooooo!)");

			if (resp->HasObject ("error") && !resp->GetObj ("error")->IsEmpty ()) {
				mStatistic.Error ("Stratum: reject error -> '" + resp->GetObj ("error")->ToString () + "'");
			}
		}
	}
}

std::string Stratum::ToHexString (const uint8_t* value, size_t size) {
	std::stringstream res;

	for (size_t i = 0; i < size; ++i) {
		res << std::hex << std::setw (2) << std::setfill ('0') << (uint32_t) value[i];
	}

	return res.str ();
}

void Stratum::LittleEndianUInt32Encode (uint8_t dest[4], uint32_t value) {
	dest[0] = value & 0xff;
	dest[1] = (value >> 8) & 0xff;
	dest[2] = (value >> 16) & 0xff;
	dest[3] = (value >> 24) & 0xff;
}

uint32_t Stratum::LittleEndianUInt32Decode (const uint8_t value[4]) {
	return ((uint32_t) (value[0]) + ((uint32_t) (value[1]) << 8) + ((uint32_t) (value[2]) << 16) + ((uint32_t) (value[3]) << 24));
}

uint32_t Stratum::BigEndianUInt32Decode (const uint8_t value[4]) {
	return ((uint32_t) (value[3]) + ((uint32_t) (value[2]) << 8) + ((uint32_t) (value[1]) << 16) + ((uint32_t) (value[0]) << 24));
}

void Stratum::GenerateJob (JobInfo& jobInfo) {
	//Init new job
	std::shared_ptr<Job> job (std::make_shared<Job> ());
	job->jobID = jobInfo.jobID;

	uint8_t* xNonce2Ptr = &jobInfo.coinBase[jobInfo.extraNonce2Pos];
	job->xNonce2.assign (xNonce2Ptr, xNonce2Ptr + mExtraNonce2Size);

	//Generate merkle root
	std::vector<uint8_t> merkleRoot (64);
	Sha2Utils::Sha256d (&merkleRoot[0], &jobInfo.coinBase[0], (int32_t) jobInfo.coinBase.size ());
	for (size_t i = 0; i < jobInfo.merkleTree.size (); i++) {
		const std::vector<uint8_t>& merkleTreeItem = jobInfo.merkleTree[i];
		memcpy (&merkleRoot[0] + 32, &merkleTreeItem[0], 32);
		Sha2Utils::Sha256d (&merkleRoot[0], &merkleRoot[0], 64);
	}

	//Increment extraNonce2
	for (uint32_t i = 0; i < mExtraNonce2Size && !++xNonce2Ptr[i]; i++) {
		//...Nothing to do...
	}

	//Assemble block header
	job->data[0] = LittleEndianUInt32Decode (&jobInfo.version[0]);
	for (uint32_t i = 0; i < 8; i++) {
		job->data[1 + i] = LittleEndianUInt32Decode (&jobInfo.prevHash[i * sizeof (uint32_t)]);
	}
	for (uint32_t i = 0; i < 8; i++) {
		job->data[9 + i] = BigEndianUInt32Decode(&merkleRoot[i * sizeof(uint32_t)]);
	}
	job->data[17] = LittleEndianUInt32Decode (&jobInfo.nTime[0]);
	job->data[18] = LittleEndianUInt32Decode (&jobInfo.nBits[0]);
	job->data[20] = 0x80000000;
	job->data[31] = 0x00000280;

	job->difficulty = mDifficulty;

	//Call reset if needed
	if (jobInfo.clean) {
		mStatistic.Message ("Stratum: Server requested work restart!");
		mWorkshop.RestartWork ();
	}

	//Set new job to workshop
	mWorkshop.AddNewJob (job);
}

Stratum::Stratum (Statistic& statistic, Workshop& workshop, std::shared_ptr<Config> cfg) :
	Network (statistic, workshop, cfg),
	mCurlClient (statistic),
	mExtraNonce2Size (0),
	mDifficulty (0)
{
}

void Stratum::Step () {
#ifdef RUN_STRATUM_TEST
	RunTest ();
#else //RUN_STRATUM_TEST
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

	//Submit all waiting job
	SubmitJobResults ();

	//Wait for server messages
	std::shared_ptr<JSONObject> json = mCurlClient.ReceiveJson (120);
	if (json == nullptr) {
		Disconnect ();
		mStatistic.Message ("Stratum: Connection timed out and interrupted!");
		return;
	}

	if (json && !HandleMethod (json)) {
		Disconnect ();
		mStatistic.Message ("Stratum: Unhandled method arrived, connection interrupted!");
		return;
	}
#endif //RUN_STRATUM_TEST
}
