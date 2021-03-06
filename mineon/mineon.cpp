// mineon.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Config.hpp"
#include "Algorythm.hpp"
#include "Workshop.hpp"
#include "Network.hpp"
#include "Statistic.hpp"

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

	//Init program state
	std::atomic_bool exitFlag = false;
	Workshop workshop;
	Statistic statistic;

	workshop.AddJobObserver ([&statistic] (const std::string& jobID) {
		statistic.NewJobArrived (jobID);
	});

	//Start mining threads
	std::vector<std::thread> miningThreads;
	for (uint32_t i = 0, iEnd = cfg->GetThreads (); i < iEnd; ++i) {
		miningThreads.push_back (std::thread ([&exitFlag, &workshop, &statistic, cfg] (uint32_t threadIndex) -> void {
			//Init mining
			std::shared_ptr<Algorythm> algorythm = Algorythm::Create (cfg->GetAlgorythmID ());
			if (algorythm == nullptr) {
				exitFlag = true;
				std::cout << "Cannot create algorythm! (Thread index: " << threadIndex << ")" << std::endl << std::endl;
				return;
			}

			//uint32_t nonceCount = 0xffffffffu / cfg->GetThreads ();
			//uint32_t nonceStart = threadIndex * nonceCount;

			//while (!workshop.HasAnyJob ()) {
			//	std::this_thread::sleep_for (std::chrono::milliseconds (500));
			//}

			uint32_t jobObserverID = workshop.AddResetObserver ([algorythm] () -> void {
				algorythm->BreakScan ();
			});

			//Run mining
			while (!exitFlag) {
				//Wait for next job
				std::shared_ptr<Job> job;
				while ((job = workshop.PopNextJob ()) == nullptr) {
					std::this_thread::sleep_for (std::chrono::milliseconds (500));
				}

				//Start the job
				if (!algorythm->Prepare (job, 0, 0xffffffffu)) {
					exitFlag = true;
					std::cout << "Cannot pepare algorythm! (Thread index: " << threadIndex << ")" << std::endl << std::endl;
					return;
				}

				Algorythm::ScanResults scanResult = algorythm->Scan (threadIndex, statistic);
				if (scanResult.foundNonce) {
					JobResult result;
					result.jobID = job->jobID;
					result.xNonce2 = job->xNonce2;

					result.nTime = scanResult.nTime;
					result.nonce = scanResult.nonce;

					workshop.SubmitJobResult (result);
				}

				std::this_thread::yield ();
			}

			//Release mining
			workshop.RemoveResetObserver (jobObserverID);
		}, i));
	}

	//Start network thread
	std::thread networkThread ([&exitFlag, &workshop, &statistic, cfg] () -> void {
		//Init network
		std::string protocol = cfg->IsBenchmark () ? "benchmark" : cfg->GetNetworkProtocol ();
		std::shared_ptr<Network> network = Network::Create (protocol, statistic, workshop, cfg);

		//Run network
		while (!exitFlag) {
			network->Step ();
			std::this_thread::yield ();
		}

		//Release network
		network->Release ();
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
