#include "file_helpers.h"

#include <random>
#include <string>
#include <future>
#include <deque>

struct TestSt {
	float x;
	float y;
};

int main(int argc, char const *argv[])
{

	spdlog::info("starting");
	std::deque<TestSt> _tmp;
	std::default_random_engine generator;
	std::uniform_real_distribution<float> distribution(1,100);
	;
	SBCQueens::DataFile<TestSt> testFile;

	SBCQueens::open(testFile, "testfile.csv");

	for(int i = 0; i < 1000000; i++) {
		TestSt st{distribution(generator), distribution(generator)};

		// Add
		testFile(st);
		_tmp.push_back(st);
	}

	SBCQueens::async_save(testFile, [](const TestSt& item) {
		return std::to_string(item.x) + "," + std::to_string(item.y) +"\n";
	});

	auto f = std::async([&]() {
		for(int i = 0; i < 1000000; i++) {
			TestSt st{distribution(generator), distribution(generator)};

			// Add
			testFile(st);
			_tmp.push_back(st);
		}
	});

	SBCQueens::async_save(testFile, [](const TestSt& item) {
		return std::to_string(item.x) + "," + std::to_string(item.y) +"\n";
	});

	spdlog::info("Finished");
	/* code */
	return 0;
}