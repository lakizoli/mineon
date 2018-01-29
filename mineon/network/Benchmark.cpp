#include "stdafx.h"
#include "Benchmark.hpp"

static uint32_t swab32 (uint32_t val) {
	return _byteswap_ulong (val);
}

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
	job.data[17] = swab32 ((uint32_t) time (nullptr));
	memset (&job.data[19], 0x00, 13 * sizeof (uint32_t));
	job.data[20] = 0x80000000;
	job.data[31] = 0x00000280;

	memset (&job.target[0], 0x00, 8 * sizeof (uint32_t));

	mWorkshop.SetNewJob (job);

	//Wait for a little bit before reset
	std::this_thread::sleep_for (std::chrono::seconds (30));
}
