// mineon.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Config.hpp"

#define OK 0
#define ERROR 1

int32_t main (int32_t argc, char* argv[]) {
	//Parse command line
	std::shared_ptr<Config> cfg = Config::ParseCommandLine (argc, argv);
	if (cfg == nullptr) {
		printf ("Empty config found!");
		return ERROR;
	}

	if (!cfg->IsValid ()) {
		cfg->ShowUsage ();
		return ERROR;
	}

	//Start mining threads
	//TODO: ...

	//Start network thread
	//TODO: ...

	//Show statistics
	//TODO: ...

	return OK;
}
