#pragma once

#include "implot_helpers.h"

namespace SBCQueens {

	enum class IndicatorNames {

		NUM_RTD_BOARDS,
		NUM_RTDS_PER_BOARD,
		IS_RTD_ONLY,

		// For plots:
		PELTIER_CURR,
		RTD_TEMP_ONE,
		RTD_TEMP_TWO,
		RTD_TEMP_THREE,

		VACUUM_PRESS,

		LOCAL_BME_Temps,
		LOCAL_BME_Pressure,
		LOCAL_BME_Humidity,

		SiPM_Plot_ZERO,
		SiPM_Plot_ONE,
		SiPM_Plot_TWO,
		SiPM_Plot_THREE,

		// For indicators:
		// Teeensy Indicators
		LATEST_RTD1_TEMP,
		LATEST_RTD2_TEMP,
		LATEST_RTD3_TEMP,
		LATEST_Peltier_CURR,

		LATEST_VACUUM_PRESS,

		// CAEN Indicators
		CAENBUFFEREVENTS,
		FREQUENCY,
		DARK_NOISE_RATE,
		GAIN,

		SAVED_WAVEFORMS
	};


	using SiPMsPlotQueue = IndicatorsQueue<IndicatorNames, double>;
	using RTDIndicator = Indicator<uint16_t>;
	using TeensyIndicator = Indicator<IndicatorNames>;
	using SiPMIndicator = Indicator<IndicatorNames>;
	using SiPMPlot = Plot<IndicatorNames>;

} // namespace SBCQueens