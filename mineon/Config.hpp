#pragma once

class Config {

	Config ();

//Construction
public:
	static std::shared_ptr<Config> ParseCommandLine (int32_t argc, char* argv[]);

//Interface
public:
	bool IsValid () const;
	void ShowUsage () const;
};
