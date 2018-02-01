#pragma once

#include "Network.hpp"
#include "CurlClient.hpp"

class Stratum : public Network {
	struct JobInfo {
		std::string jobID;
		std::vector<uint8_t> prevHash; //[32]
		std::vector<uint8_t> coinBase;
		uint32_t extraNonce2Pos;

		std::vector<std::vector<uint8_t>> merkleTree;

		std::vector<uint8_t> version; //[4]
		std::vector<uint8_t> nBits; //[4]
		std::vector<uint8_t> nTime; //[4]
		bool clean;

		JobInfo () : extraNonce2Pos (0), clean (false) {}
	};

private:
	std::string mUrl;
	CurlClient mCurlClient;

	//Stratum connection parameters
	std::string mSessionID;
	std::vector<uint8_t> mExtraNonce;
	uint32_t mExtraNonce2Size;
	double mDifficulty;
	
private:
	void RunTest ();

	bool Connect (const std::string& url);
	void Disconnect ();

	bool Subscribe ();
	bool Authorize ();

	bool HandleMethod (std::shared_ptr<JSONObject> json);
	bool HandleNotify (std::shared_ptr<JSONArray> notifyParams);

	void SubmitJobResults ();

	static std::string ToHexString (const uint8_t* value, size_t size);
	static void LittleEndianUInt32Encode (uint8_t dest[4], uint32_t value);
	static uint32_t LittleEndianUInt32Decode (const uint8_t value[4]);
	static uint32_t BigEndianUInt32Decode (const uint8_t value[4]);
	void GenerateJob (JobInfo& jobInfo);

public:
	explicit Stratum (Statistic& statistic, Workshop& workshop, std::shared_ptr<Config> cfg);

	std::string GetNetworkID () const override { return "stratum"; }
	void Step() override;
};
