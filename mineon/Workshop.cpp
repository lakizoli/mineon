#include "stdafx.h"
#include "Workshop.hpp"

uint32_t Workshop::mNextObserverID = 1;

Workshop::Workshop () :
	mHasJob (false)
{
}

void Workshop::SetNewJob (const Job& job) {
	//Reset the current job
	std::string jobID;
	{
		std::unique_lock<std::shared_mutex> lock (mSync);

		mHasJob = true;
		mJob = job;

		std::stringstream ss;
		for (uint8_t ch : mJob.jobID) {
			ss << std::hex << std::setw (2) << std::setfill ('0') << (uint32_t)ch;
		}
		jobID = ss.str ();
	}

	//Call observers
	{
		std::shared_lock<std::shared_mutex> lock (mSync);

		for (auto& it : mJobObservers) {
			it.second (jobID);
		}
	}
}

Job Workshop::GetCurrentJob () const {
	std::shared_lock<std::shared_mutex> lock (mSync);

	return mJob;
}

bool Workshop::HasJob () const {
	std::shared_lock<std::shared_mutex> lock (mSync);

	return mHasJob;
}

void Workshop::SubmitJobResult (const JobResult& result) {
	std::unique_lock<std::shared_mutex> lock (mSync);

	mSubmitJobs.push_back (result);
}

std::vector<JobResult> Workshop::GetJobResults () const {
	std::shared_lock<std::shared_mutex> lock (mSync);

	return mSubmitJobs;
}

void Workshop::RemoveSubmittedJobResult (const std::vector<uint8_t>& jobID) {
	std::unique_lock<std::shared_mutex> lock (mSync);

	auto it = std::find_if (mSubmitJobs.begin (), mSubmitJobs.end (), [&jobID] (const JobResult& result) -> bool {
		return result.jobID == jobID;
	});

	if (it != mSubmitJobs.end ()) {
		mSubmitJobs.erase (it);
	}
}

void Workshop::ClearSubmittedJobResults () {
	std::unique_lock<std::shared_mutex> lock (mSync);
	mSubmitJobs.clear ();
}

uint32_t Workshop::AddJobObserver (std::function<void (const std::string& jobID)> observer) {
	std::unique_lock<std::shared_mutex> lock (mSync);

	uint32_t observerID = mNextObserverID++;
	mJobObservers.emplace (observerID, observer);
	return observerID;
}

void Workshop::RemoveJobObserver (uint32_t observerID) {
	std::unique_lock<std::shared_mutex> lock (mSync);

	auto it = mJobObservers.find (observerID);
	if (it != mJobObservers.end ()) {
		mJobObservers.erase (it);
	}
}
