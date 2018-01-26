#include "stdafx.h"
#include "Algorythm.hpp"
#include "Scrypt.hpp"

std::vector<Algorythm::AlgorythmInfo> Algorythm::GetAvailableAlgorythms () {
	std::vector<AlgorythmInfo> algos;

	//Add scrypt to algorythms
	{
		Scrypt algo;
		algos.push_back ({ algo.GetAlgorythmID (), algo.GetDescription () });
	}

	//...

	return algos;
}

std::shared_ptr<Algorythm> Algorythm::Create (const std::string& algorythm) {
	//Check scrypt algorythm
	{
		std::shared_ptr<Algorythm> algo (new Scrypt ());
		if (algo->GetAlgorythmID () == algorythm) {
			return algo;
		}
	}

	//...

	return nullptr;
}
