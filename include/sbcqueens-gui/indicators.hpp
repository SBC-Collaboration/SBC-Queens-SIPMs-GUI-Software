#ifndef INDICATORS_H
#define INDICATORS_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <cstdint>

// C++ 3rd party includes
// my includes
#include "sbcqueens-gui/implot_helpers.hpp"

namespace SBCQueens {

enum class IndicatorNames {
    NUM_RTD_BOARDS,
    NUM_RTDS_PER_BOARD,
    IS_RTD_ONLY,

    // For plots:
    PELTIER_CURR,

    VACUUM_PRESS,
    PFEIFFER_PRESS,

    LOCAL_BME_TEMPS,
    LOCAL_BME_PRESS,
    LOCAL_BME_HUMD,

    DMM_VOLTAGE,
    PICO_CURRENT,
    GAIN_VS_VOLTAGE,

    // For indicators:
    // Teeensy Indicators
    LATEST_RTD1_TEMP,
    LATEST_RTD2_TEMP,
    LATEST_RTD3_TEMP,
    LATEST_PELTIER_CURR,
    PID_TEMP_ERROR,

    LATEST_VACUUM_PRESS,
    LATEST_DMM_VOLTAGE,
    LATEST_PICO_CURRENT,

    // CAEN Indicators
    // Boolean
    ANALYSIS_ONGOING,
    FULL_ANALYSIS_DONE,
    PROCESSING_REDUCED_WF,
    CALCULATING_VBD,

    // Numerical
    CAENBUFFEREVENTS,
    TRIGGERRATE,

    WAVEFORM_NOISE,
    WAVEFORM_BASELINE,
    SPE_GAIN_MEAN,
    SPE_EFFICIENCTY,
    INTEGRAL_THRESHOLD,
    RISE_TIME,
    FALL_TIME,
    OFFSET,
    BREAKDOWN_VOLTAGE,
    BREAKDOWN_VOLTAGE_ERR,

    SAVED_WAVEFORMS,
    DONE_DATA_TAKING
};

using GeneralIndicatorQueue = IndicatorsQueue<IndicatorNames, double>;
using MultiplePlotQueue = IndicatorsQueue<uint16_t, double>;
using SiPMPlotQueue = IndicatorsQueue<uint8_t, double>;

}  // namespace SBCQueens
#endif
