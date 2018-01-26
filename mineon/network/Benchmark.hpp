#pragma once

#include "Network.hpp"

class Benchmark : public Network {
public:
	explicit Benchmark (Workshop& workshop);

	std::string GetNetworkID () const override { return "benchmark"; }
	void Step() override;
};
