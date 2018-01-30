#pragma once

struct Job {
	std::vector<uint8_t> jobID;
	std::vector<uint8_t> xNonce2;
	std::vector<uint32_t> data; //[32];
	double difficulty;

	/**
	* Create an empty benchmark job.
	*/
	Job ();
};
