#pragma once

#include "Job.hpp"

class Workshop {
	static uint32_t mNextObserverID;

	mutable std::shared_mutex mSync;
	bool mHasJob;
	Job mJob;
	std::map<uint32_t, std::function<void (const std::string& jobID)>> mJobObservers;

	std::vector<JobResult> mSubmitJobs;

public:
	Workshop ();

	void SetNewJob (const Job& job);
	Job GetCurrentJob () const;
	bool HasJob () const;

	void SubmitJobResult (const JobResult& result);
	std::vector<JobResult> GetJobResults () const;
	void RemoveSubmittedJobResult (const std::vector<uint8_t>& jobID);
	void ClearSubmittedJobResults ();

	uint32_t AddJobObserver (std::function<void (const std::string& jobID)> observer);
	void RemoveJobObserver (uint32_t observerID);
};
