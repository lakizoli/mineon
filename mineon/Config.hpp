#pragma once

class Config {
	bool mHasValidValues;

	std::string mExeName;
	std::string mAlgorythm;
	std::string mNetworkProtocol;
	std::string mUrl;
	std::string mUser;
	std::string mPassword;

	uint32_t mThreads;
	bool mBenchmark;
	bool mShowVersion;
	bool mShowHelp;

	std::string mUserAgent;

	enum class OptionKinds {
		None,
		ShortOption,
		LongOption
	};

	Config ();
	static OptionKinds IsOption (const std::string& option, const std::string& shortOption, const std::string& longOption = "", bool checkLongStartOnly = false);
	static std::string ParseOption (int32_t argc, char* argv[], int32_t& argIndex, OptionKinds optKind, bool hasValue);

//Construction
public:
	static std::shared_ptr<Config> ParseCommandLine (int32_t argc, char* argv[]);

//Interface
public:
	bool IsValid () const {
		return mHasValidValues && !mExeName.empty () && !mAlgorythm.empty () && !mNetworkProtocol.empty () && !mUrl.empty () && !mUser.empty ();
	}

	const std::string& GetUserAgent () const {
		return mUserAgent;
	}

	void ShowVersion ();
	void ShowUsage () const;

	const std::string& GetAlgorythmID () const {
		return mAlgorythm;
	}

	const std::string& GetNetworkProtocol () const {
		return mNetworkProtocol;
	}

	const std::string& GetUrl () const {
		return mUrl;
	}

	void SetHostAndPort (const std::string& host, uint32_t port);

	const std::string& GetUser () const {
		return mUser;
	}

	const std::string& GetPassword () const {
		return mPassword;
	}

	uint32_t GetThreads () const {
		return mThreads;
	}

	bool IsBenchmark () const {
		return mBenchmark;
	}

	bool NeedShowVersion () const {
		return mShowVersion;
	}

	bool NeedShowHelp () const {
		return mShowHelp;
	}
};
