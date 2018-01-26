#include "stdafx.h"
#include "Statistic.hpp"

Statistic::Statistic () {
}

void Statistic::NewJobArrived () {
	std::cout << "Got a new job!" << std::endl;
}

void Statistic::ScanStarted (uint32_t threadIndex, std::chrono::system_clock::time_point scanStart, uint32_t startNonce, uint32_t endNonce) {
	std::cout << "Thread " << threadIndex << ": scan started (start nonce: " << startNonce << ", end nonce: " << endNonce << ")" << std::endl;
}

void Statistic::ScanStepEnded (uint32_t threadIndex, uint32_t nonce) {
	//TODO: ...
}

void Statistic::ScanEnded (uint32_t threadIndex, std::chrono::system_clock::duration duration, uint32_t hashesScanned, bool foundNonce, uint32_t nonce) {
	double secs = (double) std::chrono::duration_cast<std::chrono::seconds> (duration).count ();
	double velocity = secs == 0 ? 0 : (double) hashesScanned / secs / 1000.0;

	std::cout << "Thread " << threadIndex << ": scan ended (duration: " << secs <<
		" sec, velocity: " << velocity <<
		" kH/sec, hash count: " << hashesScanned <<
		", found: " << (foundNonce ? "true" : "false") <<
		", nonce: " << nonce << ")" << std::endl;
}
