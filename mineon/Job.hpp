#pragma once

struct Job {
	std::vector<uint32_t> data; //[32];
	std::vector<uint32_t> target; //[8];

	/**
	* Create an empty benchmark job.
	*/
	Job ();
};
