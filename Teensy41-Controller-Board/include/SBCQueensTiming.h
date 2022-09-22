#pragma once

#include <cstdint>
#include <inttypes.h>

namespace SBCQueens {

	// The RTD sampling rate. Ideally, a RTD measurement chain will be done after this time
	extern uint16_t RTDSamplingTime;

	// Should be less than RTDSamplingTime
	// Time it takes for the RTD to warm up and reach a steady state
	const uint16_t c_RTDPrepareTime = 1; //ms

	// Should be less than (RTDSamplingTime - RTDPrepareTime)
	// Time it takes for a single ADC to finalize a conversion
#ifdef NEW_RTD_BOARD
	// real is 17ms
	const uint16_t c_RTDAcquisitionTime = 20; //ms
#else 
    // The maxboard is 52
    const uint16_t c_RTDAcquisitionTime = 52; //ms
#endif


	extern uint16_t PIDUpdateTime;
	extern uint16_t BMESamplingTime;
	extern uint16_t PressureTransSamplingTime;


}

