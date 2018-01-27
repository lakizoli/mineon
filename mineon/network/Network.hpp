#pragma once

#include "Workshop.hpp"
#include "Statistic.hpp"
#include "Config.hpp"

class Network {
protected:
	Statistic& mStatistic;
	Workshop& mWorkshop;
	std::shared_ptr<Config> mConfig;

	explicit Network (Statistic& statistic, Workshop& workshop, std::shared_ptr<Config> cfg);

public:
	static std::shared_ptr<Network> Create (const std::string& networkID, Statistic& statistic, Workshop& workshop, std::shared_ptr<Config> cfg);

	virtual std::string GetNetworkID () const = 0;
	virtual void Step () = 0;
	virtual void Release () {};
};
