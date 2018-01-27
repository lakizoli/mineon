#pragma once

#include "Network.hpp"
#include "CurlClient.hpp"

class Stratum : public Network {
	std::string mUrl;
	CurlClient mCurlClient;

	std::string mSessionID;
	std::vector<uint8_t> mExtraNonce;
	uint32_t mExtraNonce2Size;

	bool Connect (const std::string& url);
	void Disconnect ();

	bool Subscribe ();
	bool Authorize ();

public:
	explicit Stratum (Statistic& statistic, Workshop& workshop, std::shared_ptr<Config> cfg);

	std::string GetNetworkID () const override { return "stratum"; }
	void Step() override;
};
