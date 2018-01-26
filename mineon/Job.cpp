#include "stdafx.h"
#include "Job.hpp"

static uint32_t swab32 (uint32_t val) {
	return _byteswap_ulong (val);
}

Job::Job () :
	jobID (1)
{
	//Allocate job
	data.resize (32);
	target.resize (8);

	//Fill benchmark data
	memset (&data[0], 0x55, 19 * sizeof (uint32_t));
	data[17] = swab32 ((uint32_t) time (nullptr));
	memset (&data[19], 0x00, 13 * sizeof (uint32_t));
	data[20] = 0x80000000;
	data[31] = 0x00000280;

	memset (&target[0], 0x00, 8 * sizeof (uint32_t));
}
