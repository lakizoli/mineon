#pragma once

#include "Job.hpp"

class Workshop {
	static uint32_t mNextObserverID;

	mutable std::shared_mutex mSync;
	bool mHasJob;
	Job mJob;
	std::map<uint32_t, std::function<void ()>> mJobObservers;

public:
	Workshop ();

	void SetNewJob (const Job& job);
	Job GetJob () const;
	bool HasJob () const;

	uint32_t AddJobObserver (std::function<void ()> observer);
	void RemoveJobObserver (uint32_t observerID);
};
