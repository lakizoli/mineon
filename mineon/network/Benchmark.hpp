#pragma once

#include "Network.hpp"

class Benchmark : public Network {
public:
	explicit Benchmark (Statistic& statistic, Workshop& workshop, std::shared_ptr<Config> cfg);

	std::string GetNetworkID () const override { return "benchmark"; }
	void Step() override;
};
