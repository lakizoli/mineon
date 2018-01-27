#include "stdafx.h"
#include "Job.hpp"

Job::Job () :
	jobID (0)
{
	data.resize (32);
	target.resize (8);
}
