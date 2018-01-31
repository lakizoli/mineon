#pragma once

struct Job {
	std::string jobID; //Hex string
	std::vector<uint8_t> xNonce2;
	std::vector<uint32_t> data; //[32];
	double difficulty;

	/**
	* Create an empty benchmark job.
	*/
	Job ();
};

struct JobResult {
	std::vector<uint8_t> jobID;
	std::vector<uint8_t> xNonce2;
	uint32_t nTime;
	uint32_t nonce;

	JobResult ();
};
