#pragma once
#include "Workshop.hpp"

class Network {
protected:
	Workshop& mWorkshop;

	explicit Network (Workshop& workshop);

public:
	static std::shared_ptr<Network> Create (std::string networkID, Workshop& workshop);

	virtual std::string GetNetworkID () const = 0;
	virtual void Step () = 0;
	virtual void Release () {};
};
