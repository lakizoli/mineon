#include "stdafx.h"
#include "Workshop.hpp"

uint32_t Workshop::mNextObserverID = 1;

Workshop::Workshop () {
}

void Workshop::AddNewJob (std::shared_ptr<Job> job) {
	//Add the new job to the list, if it is not already in the list
	std::string jobID;
	{
		std::unique_lock<std::shared_mutex> lock (mSync);

		auto it = mJobs.find (job->jobID);
		if (it == mJobs.end ()) {
			mJobs.emplace (job->jobID, job);
			jobID = job->jobID;
		}
	}

	//Call observers
	if (!jobID.empty ()) {
		std::shared_lock<std::shared_mutex> lock (mSync);

		for (auto& it : mJobObservers) {
			it.second (jobID);
		}
	}
}

std::shared_ptr<Job> Workshop::PopNextJob () {
	std::unique_lock<std::shared_mutex> lock (mSync);

	std::shared_ptr<Job> job;
	if (mJobs.size () > 0) {
		auto it = mJobs.begin ();
		job = it->second;
		mJobs.erase (it);
	}

	return job;
}

void Workshop::SubmitJobResult (const JobResult& result) {
	std::unique_lock<std::shared_mutex> lock (mSync);

	mSubmitJobs.push_back (result);
}

std::vector<JobResult> Workshop::GetJobResults () const {
	std::shared_lock<std::shared_mutex> lock (mSync);

	return mSubmitJobs;
}

void Workshop::RemoveSubmittedJobResult (const std::string& jobID) {
	std::unique_lock<std::shared_mutex> lock (mSync);

	auto it = std::find_if (mSubmitJobs.begin (), mSubmitJobs.end (), [&jobID] (const JobResult& result) -> bool {
		return result.jobID == jobID;
	});

	if (it != mSubmitJobs.end ()) {
		mSubmitJobs.erase (it);
	}
}

void Workshop::RestartWork () {
	std::unique_lock<std::shared_mutex> lock (mSync);

	mJobs.clear ();

	for (auto& it : mResetObservers) {
		it.second ();
	}
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

uint32_t Workshop::AddResetObserver (std::function<void()> observer) {
	std::unique_lock<std::shared_mutex> lock (mSync);

	uint32_t observerID = mNextObserverID++;
	mResetObservers.emplace (observerID, observer);
	return observerID;
}

void Workshop::RemoveResetObserver (uint32_t observerID) {
	std::unique_lock<std::shared_mutex> lock (mSync);

	auto it = mResetObservers.find (observerID);
	if (it != mResetObservers.end ()) {
		mResetObservers.erase (it);
	}
}

