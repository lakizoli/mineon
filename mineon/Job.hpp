#pragma once

struct Job {
	uint32_t jobID;
	std::vector<uint32_t> data; //[32];
	std::vector<uint32_t> target; //[8];

	/**
	* Create an empty benchmark job.
	*/
	Job ();
};
