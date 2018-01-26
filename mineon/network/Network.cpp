#include "stdafx.h"
#include "Network.hpp"
#include "Benchmark.hpp"

Network::Network (Workshop& workshop) :
	mWorkshop (workshop)
{
}

std::shared_ptr<Network> Network::Create (std::string networkID, Workshop& workshop) {
	//Create benchmark network communicator
	{
		std::shared_ptr<Network> net (new Benchmark (workshop));
		if (net->GetNetworkID () == networkID) {
			return net;
		}
	}

	//...

	return nullptr;
}
