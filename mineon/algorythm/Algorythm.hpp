#pragma once

class Algorythm {
//Construction interface
public:
	virtual ~Algorythm () = default;

	struct AlgorythmInfo {
		std::string id;
		std::string description;
	};

	static std::vector<AlgorythmInfo> GetAvailableAlgorythms ();
	static std::shared_ptr<Algorythm> Create (std::string algorythm);

//Algorythm interface
public:
	enum class PrepareResults {
		Unknown, ///< Unknown result.
		Error, ///< Error occured during prepare.
		HaveMore, ///< We have one more input block to prepare, because of parallel execution.
		Succeeded, ///< Prepare process succeeded.
	};

	struct ScanResults {
		//result
		bool foundNonce; ///< Flag to sign if the nonce value has been found.
		uint32_t nonce; ///< The found nonce value if any.

		//statistics
		uint32_t hashesScanned; ///< The count of scanned hashes.
		std::chrono::system_clock::time_point scanStart; ///< The start time of the scaning process.
		std::chrono::system_clock::duration scanDuration; ///< The duration of the scaning process.
	};

	/**
	* Get the ID of the algorythm.
	*/
	virtual std::string GetAlgorythmID () const = 0;

	/**
	* Get the short description of the algorythm.
	*/
	virtual std::string GetDescription () const = 0;

	/**
	* Prepare the algorithm calculation for the given nonce range.
	*/
	virtual PrepareResults Prepare (uint32_t nonceStart, uint32_t nonceCount) = 0;

	/**
	* Scan for the nonce in the prepared nonce range.
	*/
	virtual ScanResults Scan () = 0;
};
