#pragma once

class Statistic {
public:
	Statistic ();

	void NewJobArrived ();

	void ScanStarted (uint32_t threadIndex, std::chrono::system_clock::time_point scanStart, uint32_t startNonce, uint32_t endNonce);
	void ScanStepEnded (uint32_t threadIndex, uint32_t nonce);
	void ScanEnded (uint32_t threadIndex, std::chrono::system_clock::duration duration, uint32_t hashesScanned, bool foundNonce, uint32_t nonce);
};
