#pragma once

#include "Algorythm.hpp"

class Scrypt : public Algorythm {
public:
	~Scrypt () override = default;

	std::string GetAlgorythmID () const override { return "scrypt"; }
	std::string GetDescription () const override { return "scrypt(1024, 1, 1)"; }

	PrepareResults Prepare (uint32_t nonceStart, uint32_t nonceCount) override;
	ScanResults Scan () override;
};
