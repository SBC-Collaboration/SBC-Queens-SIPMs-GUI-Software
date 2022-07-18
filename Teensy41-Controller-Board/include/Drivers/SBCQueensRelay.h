#pragma once

#include <inttypes.h>

namespace SBCQueens {

	struct Relay {
		uint8_t PIN;

		// It is an uint16 for compability with the other software
		uint16_t Value;
	};

	// Sets the mode of PIN as output and initializes the pin to Value
	void relay_init(Relay&);

	// Updates the relay with current Value
	void relay_update(Relay&);

} // namespace SBCQueens