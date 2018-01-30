#include "stdafx.h"
#include "Statistic.hpp"

Statistic::Statistic () {
}

void Statistic::NewJobArrived (const std::string& jobID) {
	std::cout << "Got a new job! (jobID: " << jobID << ")" << std::endl;
}

void Statistic::ScanStarted (uint32_t threadIndex, std::chrono::system_clock::time_point scanStart, uint32_t startNonce, uint32_t endNonce) {
	std::cout << "Thread " << threadIndex << ": scan started (start nonce: " << startNonce << ", end nonce: " << endNonce << ")" << std::endl;
}

void Statistic::ScanStepEnded (uint32_t threadIndex, uint32_t nonce) {
	static struct juhu {
		uint32_t cnt;
		std::chrono::system_clock::time_point start;
		uint32_t lastNonce;
		uint64_t sum;

		juhu () {
			cnt = 0;
			start = std::chrono::system_clock::now ();
			lastNonce = 0;
			sum = 0;
		}
	} jajj;

	++jajj.cnt;
	if (jajj.cnt % 10000 == 0) {
		if (nonce > jajj.lastNonce) {
			jajj.sum += nonce - jajj.lastNonce;
			jajj.lastNonce = nonce;
		}

		std::chrono::system_clock::duration dur = std::chrono::system_clock::now () - jajj.start;
		double secs = (double) std::chrono::duration_cast<std::chrono::seconds> (dur).count ();
		double velocity = secs == 0 ? 0 : (double) jajj.sum / secs / 1000.0;

		uint32_t sumHash = (uint32_t) jajj.sum;
		std::cout << "Thread " << threadIndex << ": step ended -> (duration: " << secs <<
			" sec, velocity: " << velocity <<
			" kH/sec, hash count: " << sumHash <<
			", nonce: " << nonce << ")" << std::endl;
	}

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

void Statistic::Error (const std::string& error) {
	std::cout << "Error: " << error << std::endl;
}

void Statistic::Message (const std::string& msg) {
	std::cout << "Message: " << msg << std::endl;
}
