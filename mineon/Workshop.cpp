#include "stdafx.h"
#include "Workshop.hpp"

uint32_t Workshop::mNextObserverID = 1;

Workshop::Workshop () :
	mHasJob (false)
{
}

void Workshop::SetJob (const Job& job) {
	//Reset the current job
	{
		std::unique_lock<std::shared_mutex> lock (mSync);

		mHasJob = true;
		mJob = job;
	}

	//Call observers
	{
		std::shared_lock<std::shared_mutex> lock (mSync);

		for (auto& it : mJobObservers) {
			it.second ();
		}
	}
}

Job Workshop::GetJob () const {
	std::shared_lock<std::shared_mutex> lock (mSync);

	return mJob;
}

bool Workshop::HasJob () const {
	std::shared_lock<std::shared_mutex> lock (mSync);

	return mHasJob;
}

uint32_t Workshop::AddJobObserver (std::function<void()> observer) {
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
