#ifndef CAENINTERFACEDATA_H
#define CAENINTERFACEDATA_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ std includes
#include <functional>
// C++ 3rd party includes
#include <readerwriterqueue.h>
// my includes
#include "sbcqueens-gui/caen_helper.hpp"

namespace SBCQueens {

struct SiPMVoltageMeasure {
    double Current;
    double Volt;
    double Time;  // in unix timestamp
};

enum class CAENInterfaceStates {
    NullState = 0,
    Standby,
    AttemptConnection,
    OscilloscopeMode,
    MeasurementRoutineMode,
    Disconnected,
    Closing
};

struct BreakdownVoltageConfigData {
    uint32_t SPEEstimationTotalPulses = 20000;
    uint32_t DataPulses = 200000;
};

struct CAENInterfaceData {
    std::string RunDir = "";

    CAENConnectionType ConnectionType;

    CAENDigitizerModel Model;
    CAENGlobalConfig GlobalConfig;
    std::vector<CAENGroupConfig> GroupConfigs;

    int PortNum = 0;
    uint32_t VMEAddress = 0;

    CAENInterfaceStates CurrentState = CAENInterfaceStates::NullState;

    bool SoftwareTrigger = false;
    bool ResetCaen = false;

    std::string SiPMVoltageSysPort = "";
    int SiPMID = 0;
    int CellNumber = 1;
    float LatestTemperature = -1.f;

    bool SiPMVoltageSysChange = false;
    bool SiPMVoltageSysSupplyEN = false;
    float SiPMVoltageSysVoltage = 0.0;

    SiPMVoltageMeasure LatestMeasure;

    bool isVoltageStabilized = false;
    bool isTemperatureStabilized = false;
    bool CancelMeasurements = false;

    std::string SiPMName = "";
    BreakdownVoltageConfigData VBDData;
};

using CAENQueueType = std::function < bool(CAENInterfaceData&) >;
using CAENQueue = moodycamel::ReaderWriterQueue< CAENQueueType >;

}  // namespace SBCQueens

#endif