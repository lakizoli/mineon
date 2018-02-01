#pragma once

class Statistic {
	uint32_t mSubmittedCount;

public:
	Statistic ();

	void NewJobArrived (const std::string& jobID);

	void ScanStarted (uint32_t threadIndex, const std::string& jobID, std::chrono::system_clock::time_point scanStart, uint32_t startNonce, uint32_t endNonce);
	void ScanStepEnded (uint32_t threadIndex, const std::string& jobID, uint32_t nonce);
	void ScanEnded (uint32_t threadIndex, const std::string& jobID, std::chrono::system_clock::duration duration, uint32_t hashesScanned, bool foundNonce, uint32_t nonce);

	void Error (const std::string& error);
	void Message (const std::string& msg);
};
