#include "stdafx.h"
#include "Benchmark.hpp"

Benchmark::Benchmark (Workshop& workshop) :
	Network (workshop)
{
}

void Benchmark::Step () {
	//Set a benchmark job into the workshop
	mWorkshop.SetJob (Job ());

	//Wait for a little bit before reset
	std::this_thread::sleep_for (std::chrono::seconds (30));
}
