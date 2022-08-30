#pragma once

/*

	This class is intended to be used to hold miscellaneous devices
	which time scales are in seconds or higher.

	Example: pressure transducers

*/

namespace SBCQueens {

	struct OtherDevicesState {
		std::string RunDir 		= "";
		std::string RunName 	= "";
	};

	template<typename... Queues>
	class OtherDevicesInterface {


	};

} //namespace SBCQueens