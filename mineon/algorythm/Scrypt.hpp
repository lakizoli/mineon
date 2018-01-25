#pragma once

#include "Algorythm.hpp"
#include "Job.hpp"

class Scrypt : public Algorythm {
	Job mJob;

public:
	~Scrypt () override = default;

	std::string GetAlgorythmID () const override { return "scrypt"; }
	std::string GetDescription () const override { return "scrypt(1024, 1, 1)"; }

	bool Prepare (const Job& job, uint32_t nonceStart, uint32_t nonceCount) override;
	ScanResults Scan () override;
	void BreakScan () override;
};
