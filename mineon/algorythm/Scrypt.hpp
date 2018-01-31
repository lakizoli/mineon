#pragma once

#include "Algorythm.hpp"

#define SCRYPT_THREAD_COUNT 8

class Scrypt : public Algorythm {
	std::atomic_bool mBreakScan;

	std::string mJobID; ///< The id of the current job.
	uint32_t mStartNonce; ///< The start of the nonce range to scan.
	uint32_t mEndNonce; ///< The end of the nonce range to scan (exclusive).
	uint32_t mNonce; ///< The next available nonce value to test

	__declspec (align (32)) uint32_t mData[SCRYPT_THREAD_COUNT * 20];
	__declspec (align (32)) uint32_t mHash[SCRYPT_THREAD_COUNT * 8];
	__declspec (align (32)) uint32_t mMidState[8];
	__declspec (align (32)) uint32_t mTarget[8];

	bool FullTestHash (const uint32_t hash[8]) const;
	void DiffToTarget (double diff);

public:
	Scrypt ();
	~Scrypt () override = default;

	void* operator new (std::size_t size);
	void* operator new[] (std::size_t size);
	void operator delete (void* ptr, std::size_t size);
	void operator delete[] (void* ptr, std::size_t size);

	std::string GetAlgorythmID () const override { return "scrypt"; }
	std::string GetDescription () const override { return "scrypt(1024, 1, 1)"; }

	bool Prepare (std::shared_ptr<Job> job, uint32_t nonceStart, uint32_t nonceCount) override;
	ScanResults Scan (uint32_t threadIndex, Statistic& statistic) override;
	void BreakScan () override;
};
