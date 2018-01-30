#pragma once

#include "Network.hpp"
#include "CurlClient.hpp"

class Stratum : public Network {
	std::string mUrl;
	CurlClient mCurlClient;

	//Stratum connection and job parameters
	std::string mSessionID;
	std::vector<uint8_t> mExtraNonce;
	uint32_t mExtraNonce2Size;

	double mDifficulty;

	std::vector<uint8_t> mJobID;
	std::vector<uint8_t> mPrevHash; //[32]
	std::vector<uint8_t> mCoinBase;
	uint32_t mExtraNonce2Pos;

	std::vector<std::vector<uint8_t>> mMerkleTree;

	std::vector<uint8_t> mVersion; //[4]
	std::vector<uint8_t> mNBits; //[4]
	std::vector<uint8_t> mNTime; //[4]
	bool mClean;

	bool Connect (const std::string& url);
	void Disconnect ();

	bool Subscribe ();
	bool Authorize ();

	bool HandleMethod (std::shared_ptr<JSONObject> json);
	bool HandleNotify (std::shared_ptr<JSONArray> notifyParams);

	static uint32_t LittleEndianUInt32Decode (const uint8_t value[4]);
	static uint32_t BigEndianUInt32Decode (const uint8_t value[4]);
	static void DiffToTarget (double diff, std::vector<uint32_t>& target);
	void GenerateJob ();

public:
	explicit Stratum (Statistic& statistic, Workshop& workshop, std::shared_ptr<Config> cfg);

	std::string GetNetworkID () const override { return "stratum"; }
	void Step() override;
};
