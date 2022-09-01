#pragma once

/*

	This class is intended to be used to hold miscellaneous devices
	which time scales are in seconds or higher.

	Example: pressure transducers

*/

// std includes
#include <string>

// 3rd party includes


// project includes

namespace SBCQueens {

	enum class PFEIFFERSingleGaugeSP {
		SLOW, 	// 1 min
		FAST, 	// 1s
		FASTER	// 100ms
	};

	struct OtherDevicesData {
		std::string RunDir 		= "";
		std::string RunName 	= "";

		bool PFEIFFERSingleGaugeEnable = false;
		PFEIFFERSingleGaugeSP PFEIFFERSingleGaugeUpdateSpeed
			= PFEIFFERSingleGaugeSP::SLOW;
	};

	template<typename... Queues>
	class OtherDevicesInterface {


	};

} //namespace SBCQueens