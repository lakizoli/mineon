#include "stdafx.h"
#include "Job.hpp"

Job::Job () :
	difficulty (0.0)
{
	data.resize (32);
}

JobResult::JobResult () :
	nTime (0),
	nonce (0)
{
}
