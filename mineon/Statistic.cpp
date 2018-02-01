#include "stdafx.h"
#include "Statistic.hpp"

Statistic::Statistic () :
	mSubmittedCount (0)
{
}

void Statistic::NewJobArrived (const std::string& jobID) {
	std::cout << std::endl;
	std::cout << "Got a new job! (jobID: " << jobID << ")" << std::endl;
}

void Statistic::ScanStarted (uint32_t threadIndex, const std::string& jobID, std::chrono::system_clock::time_point scanStart, uint32_t startNonce, uint32_t endNonce) {
	std::cout << std::endl;
	std::cout << "Thread " << threadIndex << " (" << jobID << "): scan started (start nonce: " << startNonce << ", end nonce: " << endNonce << ")" << std::endl;
}

void Statistic::ScanStepEnded (uint32_t threadIndex, const std::string& jobID, uint32_t nonce) {
	//TODO: ...

	static uint32_t cnt = 0;
	++cnt;

#ifdef _DEBUG
	if (cnt % 100 == 0) {
#else //_DEBUG
	if (cnt % 1000 == 0) {
#endif //_DEBUG
		std::cout << "Thread " << threadIndex << " (" << jobID << "): step ended -> nonce: " << nonce << "       \r";
	}
}

void Statistic::ScanEnded (uint32_t threadIndex, const std::string& jobID, std::chrono::system_clock::duration duration, uint32_t hashesScanned, bool foundNonce, uint32_t nonce) {
	double secs = (double) std::chrono::duration_cast<std::chrono::seconds> (duration).count ();
	double velocity = secs == 0 ? 0 : (double) hashesScanned / secs / 1000.0;

	std::cout << std::endl;
	std::cout << "Thread " << threadIndex << " (" << jobID << "): scan ended (duration: " << secs <<
		" sec, velocity: " << velocity <<
		" kH/sec, hash count: " << hashesScanned <<
		", found: " << (foundNonce ? "true" : "false") <<
		", nonce: " << nonce << ")" << std::endl;

	if (foundNonce) {
		++mSubmittedCount;
		std::cout << "Submitted work count: " << mSubmittedCount;
	}
}

void Statistic::Error (const std::string& error) {
	std::cout << "Error: " << error << std::endl;
}

void Statistic::Message (const std::string& msg) {
	std::cout << "Message: " << msg << std::endl;
}
