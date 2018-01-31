#pragma once

#include "Job.hpp"

class Workshop {
	static uint32_t mNextObserverID;

	mutable std::shared_mutex mSync;
	std::map<std::string, std::shared_ptr<Job>> mJobs;
	std::map<uint32_t, std::function<void (const std::string& jobID)>> mJobObservers;
	std::map<uint32_t, std::function<void ()>> mResetObservers;

	std::vector<JobResult> mSubmitJobs;

public:
	Workshop ();

	void AddNewJob (std::shared_ptr<Job> job);
	std::shared_ptr<Job> PopNextJob ();

	void SubmitJobResult (const JobResult& result);
	std::vector<JobResult> GetJobResults () const;
	void RemoveSubmittedJobResult (const std::vector<uint8_t>& jobID);
	void ClearSubmittedJobResults ();

	uint32_t AddJobObserver (std::function<void (const std::string& jobID)> observer);
	void RemoveJobObserver (uint32_t observerID);

	uint32_t AddResetObserver (std::function<void ()> observer);
	void RemoveResetObserver (uint32_t observerID);
};
