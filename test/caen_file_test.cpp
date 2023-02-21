// g++ caen_file_test.cpp ../src/caen_helper.cpp -Og -I"X:/Program Files/CAEN/Comm/include" -I"X:/Program Files/CAEN/VME/include" -I"X:/Program Files/CAEN/Digitizers/Library/include" -I../include -I../deps/concurrentqueue -L"X:/Program Files/CAEN/Comm/lib" -L"X:/Program Files/CAEN/VME/lib" -L"X:/Program Files/CAEN/Digitizers/Library/lib" -lCAENDigitizer -o out.exe -static-libstdc++ -g
// g++ caen_file_test.cpp ../src/caen_helper.cpp -Og -I"C:/Program Files/CAEN/Comm/include" -I"C:/Program Files/CAEN/VME/include" -I"C:/Program Files/CAEN/Digitizers/Library/include" -I../include -I../deps/concurrentqueue -L"X:/Program Files/CAEN/Comm/lib" -L"C:/Program Files/CAEN/VME/lib" -L"C:/Program Files/CAEN/Digitizers/Library/lib" -lCAENDigitizer -o out.exe -static-libstdc++ -g
#include "caen_helper.h"
#include "file_helpers.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <random>

int main(int argc, char const *argv[])
{
	// This should never done in an actual production code
	// as no actual digitizer will be associated with this
	SBCQueens::CAEN res = SBCQueens::CAEN(
		SBCQueens::CAENDigitizerModel::DT5730B,
		CAEN_DGTZ_ConnectionType::A4818,
		23473, 0, 0, 0
	);

	res->GlobalConfig = SBCQueens::CAENGlobalConfig {
		.MaxEventsPerRead = 512,
		.RecordLength = 1200,
	};

	res->GroupConfigs[0] = SBCQueens::CAENGroupConfig {
		.Number = 3,
		.TriggerMask = 1,
		.DCOffset = 0x4534,
		.DCRange = 1,
		.TriggerThreshold = 7500,

	};

	res->GroupConfigs[1] = SBCQueens::CAENGroupConfig {
		.Number = 5,
		.TriggerMask = 1,
		.DCOffset = 0x4569,
		.DCRange = 0,
		.TriggerThreshold = 7200,

	};

	SBCQueens::DataFile<SBCQueens::CAENEvent> testFile;

	open(testFile, "test.bin", SBCQueens::sbc_init_file, res);

	SBCQueens::CAENEvent evt = std::make_shared<SBCQueens::caenEvent>(
		res->Handle
	);

	evt->Data = new CAEN_DGTZ_UINT16_EVENT_t;
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
	std::uniform_int_distribution<uint16_t> distribution(0, 0x2000);

	if(evt->Data) {
		evt->Data->ChSize[3] = 1200;
		evt->Data->DataChannel[3] = new uint16_t[1200];

		evt->Data->ChSize[5] = 1200;
		evt->Data->DataChannel[5] = new uint16_t[1200];

		evt->Info.TriggerTimeTag = 50;
		evt->Info.Pattern = 42;

		for(size_t i = 0; i < 1200; i++) {

			evt->Data->DataChannel[3][i] = distribution(gen);
			evt->Data->DataChannel[5][i] = distribution(gen);

		}
	}

	testFile->Add(evt);

	save(testFile, SBCQueens::sbc_save_func, res);

	return 0;
}