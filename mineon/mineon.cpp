// mineon.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Config.hpp"
#include "Algorythm.hpp"

#define MINEON_OK 0
#define MINEON_ERROR 1

int32_t main (int32_t argc, char* argv[]) {
	//Parse command line
	std::shared_ptr<Config> cfg = Config::ParseCommandLine (argc, argv);
	if (cfg == nullptr) {
		std::cout << "Empty config found!" << std::endl << std::endl;
		return MINEON_ERROR;
	}

	if (!cfg->IsValid ()) {
		cfg->ShowUsage ();
		return MINEON_ERROR;
	}

	//Handle simple commandline functions
	if (cfg->NeedShowHelp ()) {
		cfg->ShowUsage ();
		return MINEON_OK;
	}

	if (cfg->NeedShowVersion ()) {
		cfg->ShowVersion ();
		return MINEON_OK;
	}

	//Start mining threads
	std::atomic_bool exitFlag = false;
	std::vector<std::thread> miningThreads;
	for (uint32_t i = 0, iEnd = cfg->GetThreads (); i < iEnd; ++i) {
		miningThreads.push_back (std::thread ([&exitFlag, cfg] (uint32_t threadID) -> void {
			//Init mining
			std::shared_ptr<Algorythm> algorythm = Algorythm::Create (cfg->GetAlgorythmID ());
			if (algorythm == nullptr) {
				exitFlag = true;
				std::cout << "Cannot create algorythm!" << std::endl << std::endl;
				return;
			}

			//Run mining
			while (!exitFlag) {
				//TODO: ...
				std::this_thread::yield ();
			}

			//Release mining
			//TODO: ...
		}, i + 1));
	}

	//Start network thread
	std::thread networkThread ([&exitFlag] () -> void {
		//Init network
		//TODO: ...

		//Run network
		while (!exitFlag) {
			//TODO: ...
			std::this_thread::yield ();
		}

		//Release network
		//TODO: ...
	});

	//Show statistics
	while (!exitFlag) {
		//TODO: ...

		std::this_thread::sleep_for (std::chrono::milliseconds (500));
	}

	//Exit
	networkThread.join ();

	for (auto& thread : miningThreads) {
		thread.join ();
	}

	return MINEON_OK;
}
