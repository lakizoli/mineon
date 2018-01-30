#include "stdafx.h"
#include "Job.hpp"

Job::Job () :
	jobID (0),
	difficulty (0.0)
{
	data.resize (32);
}
