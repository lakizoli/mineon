#include "stdafx.h"
#include "Stratum.hpp"
#include "Json.hpp"
#include "CurlClient.hpp"
#include "Sha2Utils.hpp"

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

		mJobID.clear ();
		mPrevHash.clear ();
		mCoinBase.clear ();
		mExtraNonce2Pos = 0;

		mMerkleTree.clear ();

		mVersion.clear ();
		mNBits.clear ();
		mNTime.clear ();
		mClean = false;

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
	//Clear stratum values
	mSessionID.clear ();
	mExtraNonce.clear ();
	mExtraNonce2Size = 0;

	mDifficulty = 0.0;

	mJobID.clear ();
	mPrevHash.clear ();
	mCoinBase.clear ();
	mExtraNonce2Pos = 0;

	mMerkleTree.clear ();

	mVersion.clear ();
	mNBits.clear ();
	mNTime.clear ();
	mClean = false;

	//Parse parameters
	std::string jobID = notifyParams->GetStringAtIndex (0);
	if (jobID.empty ()) {
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

	mClean = notifyParams->GetBoolAtIndex (8);

	//Convert job id
	for (size_t i = 0, iEnd = jobID.length (); i < iEnd; i += 2) {
		mJobID.push_back ((uint8_t) stoul (jobID.substr (i, 2), 0, 16));
	}

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

		mMerkleTree.push_back (merkleItem);
	}

	//Compose coinbase -> [coinbase1] + [xnonce1] + [xnonce2] + [coinbase2]
	mExtraNonce2Pos = (uint32_t) (coinBase1.length () / 2 + mExtraNonce.size ());
	for (size_t i = 0, iEnd = coinBase1.length (); i < iEnd; i += 2) {
		mCoinBase.push_back ((uint8_t) stoul (coinBase1.substr (i, 2), 0, 16));
	}
	std::copy (mExtraNonce.begin (), mExtraNonce.end (), std::back_inserter (mCoinBase));
	for (size_t i = 0; i < mExtraNonce2Size; ++i) {
		mCoinBase.push_back (0);
	}
	for (size_t i = 0, iEnd = coinBase2.length (); i < iEnd; i += 2) {
		mCoinBase.push_back ((uint8_t) stoul (coinBase2.substr (i, 2), 0, 16));
	}

	//Convert prevHash
	for (size_t i = 0, iEnd = prevHash.length (); i < iEnd; i += 2) {
		mPrevHash.push_back ((uint8_t) stoul (prevHash.substr (i, 2), 0, 16));
	}

	//Convert version
	for (size_t i = 0, iEnd = version.length (); i < iEnd; i += 2) {
		mVersion.push_back ((uint8_t) stoul (version.substr (i, 2), 0, 16));
	}

	//Convert nbits
	for (size_t i = 0, iEnd = nbits.length (); i < iEnd; i += 2) {
		mNBits.push_back ((uint8_t) stoul (nbits.substr (i, 2), 0, 16));
	}

	//Convert ntime
	for (size_t i = 0, iEnd = ntime.length (); i < iEnd; i += 2) {
		mNTime.push_back ((uint8_t) stoul (ntime.substr (i, 2), 0, 16));
	}

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

		std::string jobID = ToHexString (&result.jobID[0], result.jobID.size ());

		std::string xNonce2;
		if (result.xNonce2.size () > 0) {
			xNonce2 = ToHexString (&result.xNonce2[0], result.xNonce2.size ());
		}

		std::shared_ptr<JSONArray> params = JSONArray::Create ();
		params->Add (mConfig->GetUser ());
		params->Add (jobID);
		params->Add (xNonce2);
		params->Add (nTime);
		params->Add (nonce);
		req->Add ("params", params);

		//Execute call
		std::shared_ptr<JSONObject> resp = mCurlClient.CallJsonRPC (req);

		//Handle response
		succeeded = resp->HasBool ("result") && resp->GetBool ("result");
		if (succeeded) {
			mWorkshop.RemoveSubmittedJobResult (result.jobID);
		} else { //Failed or rejected
			mStatistic.Error ("Stratum: submitted work rejected!");

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

void Stratum::DiffToTarget (double diff, std::vector<uint32_t>& target) {
	int32_t k = 0;
	for (k = 6; k > 0 && diff > 1.0; k--) {
		diff /= 4294967296.0;
	}
	uint64_t m = 4294901760.0 / diff;
	if (m == 0 && k == 6) {
		memset (&target[0], 0xff, 8 * sizeof (uint32_t));
	} else {
		memset (&target[0], 0, 8 * sizeof (uint32_t));
		target[k] = (uint32_t) m;
		target[k + 1] = (uint32_t) (m >> 32);
	}
}

void Stratum::GenerateJob () {
	//Init new job
	Job job;
	job.jobID = mJobID;

	uint8_t* xNonce2Ptr = &mCoinBase[mExtraNonce2Pos];
	job.xNonce2.assign (xNonce2Ptr, xNonce2Ptr + mExtraNonce2Size);

	//Generate merkle root
	std::vector<uint8_t> merkleRoot (64);
	Sha2Utils::Sha256d (&merkleRoot[0], &mCoinBase[0], (int32_t) mCoinBase.size ());
	for (size_t i = 0; i < mMerkleTree.size (); i++) {
		const std::vector<uint8_t>& merkleTreeItem = mMerkleTree[i];
		memcpy (&merkleRoot[0] + 32, &merkleTreeItem[0], 32);
		Sha2Utils::Sha256d (&merkleRoot[0], &merkleRoot[0], 64);
	}

	//Increment extraNonce2
	for (uint32_t i = 0; i < mExtraNonce2Size && !++xNonce2Ptr[i]; i++) {
		//...Nothing to do...
	}

	//Assemble block header
	job.data[0] = LittleEndianUInt32Decode (&mVersion[0]);
	for (uint32_t i = 0; i < 8; i++) {
		job.data[1 + i] = LittleEndianUInt32Decode (&mPrevHash[i * sizeof (uint32_t)]);
	}
	for (uint32_t i = 0; i < 8; i++) {
		job.data[9 + i] = BigEndianUInt32Decode(&merkleRoot[i * sizeof(uint32_t)]);
	}
	job.data[17] = LittleEndianUInt32Decode (&mNTime[0]);
	job.data[18] = LittleEndianUInt32Decode (&mNBits[0]);
	job.data[20] = 0x80000000;
	job.data[31] = 0x00000280;

	job.difficulty = mDifficulty;

	//Set new job to workshop
	mWorkshop.SetNewJob (job);
}

Stratum::Stratum (Statistic& statistic, Workshop& workshop, std::shared_ptr<Config> cfg) :
	Network (statistic, workshop, cfg),
	mCurlClient (statistic),
	mExtraNonce2Size (0),
	mDifficulty (0),
	mExtraNonce2Pos (0),
	mClean (false)
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

	//Set the new job if any
	bool newJobArrived = true;
	if (mWorkshop.HasJob ()) {
		Job job = mWorkshop.GetCurrentJob ();
		newJobArrived = job.jobID != mJobID;
	}

	if (newJobArrived) {
		GenerateJob ();

		if (mClean) {
			mStatistic.Message ("Stratum: Server requested work restart!");
			mWorkshop.ClearSubmittedJobResults ();
		}
	}

	//Submit all waiting job
	SubmitJobResults ();

	//Wait for server messages
	if (!mCurlClient.WaitNextMessage (120)) {
		Disconnect ();
		mStatistic.Message ("Stratum: Connection timed out and interrupted!");
		return;
	}

	std::shared_ptr<JSONObject> json = mCurlClient.ReceiveJson ();

	//Handle response
	if (json && HandleMethod (json)) {
		//json = mCurlClient.ReceiveJson ();
	}
}
