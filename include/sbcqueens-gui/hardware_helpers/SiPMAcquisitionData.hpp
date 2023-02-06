#ifndef CAENINTERFACEDATA_H
#define CAENINTERFACEDATA_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ std includes
// C++ 3rd party includes

// my includes
#include "sbcqueens-gui/multithreading_helpers/Pipe.hpp"

#include "sbcqueens-gui/caen_helper.hpp"
#include "sbcqueens-gui/imgui_helpers.hpp"

namespace SBCQueens {

struct SiPMVoltageMeasure {
    double Current;
    double Volt;
    double Time;  // in unix timestamp
};

enum class SiPMAcquisitionStates {
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

struct SiPMAcquisitionData;

// Multi-threading items
using SiPMAcquisitionDataPipeCallback = std::function<void(SiPMAcquisitionData&)>;

// It accepts any Queue with a FIFO style.
template<template<typename, typename> class QueueType,
         typename Traits,
         typename TokenType>
struct SiPMAcquisitionPipe :
    public Pipe<QueueType, SiPMAcquisitionData, Traits, TokenType> { };

template<class TPipe, PipeEndType Type>
struct SiPMAcquisitionPipeEnd : public PipeEnd<TPipe, Type> {
    explicit SiPMAcquisitionPipeEnd(TPipe p) : PipeEnd<TPipe, Type>(p) {}
};

// CAEN Interface data that holds every non-volatile items.
struct SiPMAcquisitionData {
    std::string RunDir = "";

    CAENConnectionType ConnectionType;

    CAENDigitizerModel Model;
    CAENGlobalConfig GlobalConfig;
    std::vector<CAENGroupConfig> GroupConfigs;

    int PortNum = 0;
    uint32_t VMEAddress = 0;

    SiPMAcquisitionStates CurrentState = SiPMAcquisitionStates::NullState;

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

    // Indicator/"Out" data members
    uint32_t FileStatistics = 0;

    // This API required items.
    bool Changed = false;
    SiPMAcquisitionDataPipeCallback Callback;
};


// Control and indicators
template<ControlTypes ControlType, StringLiteral Label>
using SiPMAcquisitionControl = Control<ControlType, Label>;

}  // namespace SBCQueens

#endif