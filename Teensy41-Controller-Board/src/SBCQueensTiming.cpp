#include "SBCQueensTiming.h"

#include "SBCQueensTeensyBasics.h"

namespace SBCQueens
{
    // The RTD sampling rate. Ideally, a RTD measurement chain will be done after this time.
	uint16_t RTDSamplingTime = 100; //ms

    uint16_t PIDUpdateTime = RTDSamplingTime;
    uint16_t BMESamplingTime = 114; //ms
	uint16_t PressureTransSamplingTime = 5; //ms

} // namespace SBCQueens
