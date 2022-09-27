#ifndef INDICATORS_H
#define INDICATORS_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <cstdint>

// C++ 3rd party includes
// my includes
#include "implot_helpers.hpp"


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
        PFEIFFER_PRESS,

        LOCAL_BME_Temps,
        LOCAL_BME_Pressure,
        LOCAL_BME_Humidity,

        DMM_VOLTAGE,
        PICO_CURRENT,

        // For indicators:
        // Teeensy Indicators
        LATEST_RTD1_TEMP,
        LATEST_RTD2_TEMP,
        LATEST_RTD3_TEMP,
        LATEST_Peltier_CURR,

        LATEST_VACUUM_PRESS,
        LATEST_DMM_VOLTAGE,
        LATEST_PICO_CURRENT,

        // CAEN Indicators
        CAENBUFFEREVENTS,
        TRIGGERRATE,
        FREQUENCY,
        DARK_NOISE_RATE,
        GAIN,

        SAVED_WAVEFORMS
    };


    using GeneralIndicatorQueue = IndicatorsQueue<IndicatorNames, double>;
    using MultiplePlotQueue = IndicatorsQueue<uint16_t, double>;
    using SiPMPlotQueue = IndicatorsQueue<uint8_t, double>;

    // using RTDIndicator = Indicator<uint16_t>;
    // using TeensyIndicator = Indicator<IndicatorNames>;
    // using SiPMIndicator = Indicator<IndicatorNames>;
    // using SiPMPlot = Plot<IndicatorNames>;

}  // namespace SBCQueens
#endif
