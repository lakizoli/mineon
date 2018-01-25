#include "stdafx.h"
#include "Config.hpp"
#include "Algorythm.hpp"

Config::Config () :
	mHasValidValues (true),
	mThreads (1),
	mBenchmark (false),
	mShowVersion (false),
	mShowHelp (false)
{
}

Config::OptionKinds Config::IsOption (const std::string& option, const std::string& shortOption, const std::string& longOption, bool checkLongStartOnly) {
	//Short option comparison
	if (option == shortOption) {
		return OptionKinds::ShortOption;
	}

	//Full long option comparison
	if (!checkLongStartOnly && option == longOption) {
		return OptionKinds::LongOption;
	}

	//Partial long option comparison
	if (option.length () >= longOption.length () && option.substr (0, longOption.length ()) == longOption) {
		return OptionKinds::LongOption;
	}

	return OptionKinds::None;
}

std::string Config::ParseOption (int32_t argc, char* argv[], int32_t& argIndex, OptionKinds optKind, bool hasValue) {
	switch (optKind) {
	case OptionKinds::ShortOption: {
		if (hasValue) {
			if (argIndex + 1 >= argc) { //No value available
				break; //Invalid value
			}

			++argIndex;
			return argv[argIndex];
		} 
		
		//Only boolean options have short form without value
		return "true";
	}
	case OptionKinds::LongOption: {
		break;
	}
	default:
		break;
	}

	return std::string ();
}

std::shared_ptr<Config> Config::ParseCommandLine (int32_t argc, char* argv[]) {
	std::shared_ptr<Config> cfg (new Config ());
	if (argc > 0) { //If we have valid exe name
		std::experimental::filesystem::path exePath (argv[0]);
		cfg->mExeName = exePath.filename ().string ();

		//Check value
		if (cfg->mExeName.empty ()) {
			cfg->mHasValidValues = false;
		}

		//Parse parameters
		for (int32_t i = 1; i < argc && cfg->mHasValidValues; ++i) {
			std::string opt = argv[i];
			OptionKinds optKind = OptionKinds::None;

			if ((optKind = IsOption (opt, "-a", "--algo=", true)) != OptionKinds::None) {
				cfg->mAlgorythm = ParseOption (argc, argv, i, optKind, true);

				//Check value
				std::vector<Algorythm::AlgorythmInfo> algos = Algorythm::GetAvailableAlgorythms ();
				bool found = false;
				for (const Algorythm::AlgorythmInfo& algo : algos) {
					if (algo.id == cfg->mAlgorythm) {
						found = true;
						break;
					}
				}

				if (!found) {
					cfg->mHasValidValues = false;
				}
			} else if ((optKind = IsOption (opt, "-o", "--url=", true)) != OptionKinds::None) {
				cfg->mUrl = ParseOption (argc, argv, i, optKind, true);

				//Check value
				if (cfg->mUrl.empty ()) {
					cfg->mHasValidValues = false;
				}
			} else if ((optKind = IsOption (opt, "-O", "--userpass=", true)) != OptionKinds::None) {
				std::string userpass = ParseOption (argc, argv, i, optKind, true);
				size_t posSeparator = userpass.find (':');
				if (posSeparator != std::string::npos) {
					cfg->mUser = userpass.substr (0, posSeparator);
					if (userpass.length () > posSeparator + 1) {
						cfg->mPassword = userpass.substr (posSeparator + 1);
					}
				}

				//Check value
				if (cfg->mUser.empty ()) {
					cfg->mHasValidValues = false;
				}
			} else if ((optKind = IsOption (opt, "-u", "--user=", true)) != OptionKinds::None) {
				cfg->mUser = ParseOption (argc, argv, i, optKind, true);

				//Check value
				if (cfg->mUser.empty ()) {
					cfg->mHasValidValues = false;
				}
			} else if ((optKind = IsOption (opt, "-p", "--pass=", true)) != OptionKinds::None) {
				cfg->mPassword = ParseOption (argc, argv, i, optKind, true);
			} else if ((optKind = IsOption (opt, "-t", "--threads=", true)) != OptionKinds::None) {
				cfg->mThreads = (uint32_t) std::stoul (ParseOption (argc, argv, i, optKind, true));

				//Check value
				if (cfg->mThreads < 1) {
					cfg->mHasValidValues = false;
				}
			} else if ((optKind = IsOption (opt, "", "--benchmark", false)) != OptionKinds::None) {
				cfg->mBenchmark = ParseOption (argc, argv, i, optKind, false) == "true";
			} else if ((optKind = IsOption (opt, "-V", "--version", false)) != OptionKinds::None) {
				cfg->mShowVersion = ParseOption (argc, argv, i, optKind, false) == "true";
			} else if ((optKind = IsOption (opt, "-h", "--help", false)) != OptionKinds::None) {
				cfg->mShowHelp= ParseOption (argc, argv, i, optKind, false) == "true";
			}
		}
	}

	return cfg;
}

void Config::ShowVersion () {
	std::cout << "mineon 1.0.0" << std::endl << std::endl;
}

void Config::ShowUsage () const {
	std::cout <<
		"Usage: " << mExeName << " [OPTIONS]" << std::endl <<
		"Options:" << std::endl <<
		"  -a, --algo=ALGO       specify the algorithm to use" << std::endl <<
		"                          scrypt    scrypt(1024, 1, 1) (default)" << std::endl <<
		"  -o, --url=URL         URL of mining server" << std::endl <<
		"  -O, --userpass=U:P    username:password pair for mining server" << std::endl <<
		"  -u, --user=USERNAME   username for mining server" << std::endl <<
		"  -p, --pass=PASSWORD   password for mining server" << std::endl <<
		"  -t, --threads=N       number of miner threads (default: number of processors)" << std::endl <<
		"  -B, --background      run the miner in the background" << std::endl <<
		"      --benchmark       run in offline benchmark mode" << std::endl <<
		"  -V, --version         display version information and exit" << std::endl <<
		"  -h, --help            display this help text and exit" << std::endl <<
		std::endl;
}
