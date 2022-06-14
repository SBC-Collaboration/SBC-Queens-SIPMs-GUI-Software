#pragma once

#include "implot_helpers.h"

namespace SBCQueens {

	enum class IndicatorNames {
		// For plots:
		PELTIER_CURR,
		RTD_TEMP_ONE,
		RTD_TEMP_TWO,
		VACUUM_PRESS,
		NTWO_PRESS,
		LOCAL_BME_Temps,
		LOCAL_BME_Pressure,
		LOCAL_BME_Humidity,
		BOX_BME_Temps,
		BOX_BME_Pressure,
		BOX_BME_Humidity,

		SiPM_Plot_ZERO,
		SiPM_Plot_ONE,
		SiPM_Plot_TWO,
		SiPM_Plot_THREE,

		// For indicators:
		// Teeensy Indicators
		LATEST_RTD1_TEMP,
		LATEST_RTD2_TEMP,
		LATEST_Peltier_CURR,

		LATEST_VACUUM_PRESS,
		LATEST_N2_PRESS,

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