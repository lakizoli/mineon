#include "stdafx.h"
#include "Benchmark.hpp"

Benchmark::Benchmark (Statistic& statistic, Workshop& workshop, std::shared_ptr<Config> cfg) :
	Network (statistic, workshop, cfg)
{
}

void Benchmark::Step () {
	//Set a benchmark job into the workshop
	Job job;

	//Fill benchmark data
	job.jobID = { 1 };

	memset (&job.data[0], 0x55, 19 * sizeof (uint32_t));
	job.data[17] = _byteswap_ulong ((uint32_t) time (nullptr)); //swab32 ==> _byteswap_ulong
	memset (&job.data[19], 0x00, 13 * sizeof (uint32_t));
	job.data[20] = 0x80000000;
	job.data[31] = 0x00000280;

	job.difficulty = 0.0;

	mWorkshop.SetNewJob (job);

	//Wait for a little bit before reset
	std::this_thread::sleep_for (std::chrono::seconds (30));
}
