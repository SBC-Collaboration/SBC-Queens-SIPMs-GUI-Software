#pragma once

#include "implot_helpers.h"

namespace SBCQueens {

	enum class IndicatorNames {
		// For plots:
		PID1_Temps,
		PID1_Currs,
		PID2_Temps,
		PID2_Currs,
		LOCAL_BME_Temps,
		LOCAL_BME_Pressure,
		LOCAL_BME_Humidity,
		BOX_BME_Temps,
		BOX_BME_Pressure,
		BOX_BME_Humidity,
		SiPM_Plot,

		// For indicators:
		// Teeensy Indicators
		LATEST_PID1_TEMP,
		LATEST_PID1_CURR,
		LATEST_PID2_TEMP,
		LATEST_PID2_CURR,
		LATEST_BOX_BME_HUM,
		LATEST_BOX_BME_TEMP,

		// CAEN Indicators
		CAENBUFFEREVENTS,
		FREQUENCY,
		DARK_NOISE_RATE,
		GAIN
	};


	using SiPMsPlotQueue = IndicatorsQueue<IndicatorNames, double>;
	using TeensyIndicator = Indicator<IndicatorNames>;
	using SiPMIndicator = Indicator<IndicatorNames>;
	using SiPMPlot = Plot<IndicatorNames>;

} // namespace SBCQueens