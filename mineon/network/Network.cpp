#include "stdafx.h"
#include "Network.hpp"
#include "Benchmark.hpp"
#include "Stratum.hpp"

Network::Network (Statistic& statistic, Workshop& workshop, std::shared_ptr<Config> cfg) :
	mStatistic (statistic),
	mWorkshop (workshop),
	mConfig (cfg)
{
}

std::shared_ptr<Network> Network::Create (const std::string& networkID, Statistic& statistic, Workshop& workshop, std::shared_ptr<Config> cfg) {
	//Create benchmark network communicator
	{
		std::shared_ptr<Network> net (new Benchmark (statistic, workshop, cfg));
		if (net->GetNetworkID () == networkID) {
			return net;
		}
	}

	//Create stratum network communicator
	{
		std::shared_ptr<Network> net (new Stratum (statistic, workshop, cfg));
		if (net->GetNetworkID () == networkID) {
			return net;
		}
	}

	//...

	return nullptr;
}
