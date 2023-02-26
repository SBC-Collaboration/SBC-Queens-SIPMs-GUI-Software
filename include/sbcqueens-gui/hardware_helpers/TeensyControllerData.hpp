#ifndef TEENSYCONTROLLERDATA_H
#define TEENSYCONTROLLERDATA_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ std includes
// C++ 3rd party includes
// my includes
#include "sbcqueens-gui/multithreading_helpers/Pipe.hpp"

#include "sbcqueens-gui/imgui_helpers.hpp"
#include "sbcqueens-gui/implot_helpers.hpp"

namespace SBCQueens {

enum class TeensyControllerStates {
    Standby,
    AttemptConnection,
    Connected,
    Disconnected,
    Closing
};

enum class PIDState {
    Standby,
    Running
};

struct PIDConfig {
    float SetPoint = 0.0;
    float Kp = 0.0;
    float Ti = 0.0;
    float Td = 0.0;
};

// Sensors structs
struct PeltierValues {
    double Current;
};

struct Peltiers {
    double time;
    PeltierValues PID;
};

struct RTDs {
    double time;
    std::vector<double> RTDS;
};

struct RawRTDs {
    double time;
    std::vector<uint16_t> RTDREGS;
    std::vector<double> Resistances;
    std::vector<double> Temps;
};

struct PressureValues {
    double Pressure;
};

struct Pressures {
    double time;
    PressureValues Vacuum;
};


struct TeensySystemPars {
    uint32_t NumRtdBoards = 0;
    uint32_t NumRtdsPerBoard = 0;
    bool InRTDOnlyMode = false;
};

enum class TeensyCommands {
    CheckError,
    GetError,
    Reset,

    SetPPIDUpdatePeriod,
    SetPPIDRTD,
    SetPPID,
    SetPPIDTripPoint,
    SetPPIDTempSetpoint,
    SetPPIDTempKp,
    SetPPIDTempTi,
    SetPPIDTempTd,
    ResetPPID,

    SetRTDSamplingPeriod,
    SetRTDMask,

    SetPeltierRelay,

    GetSystemParameters,
    GetPeltiers,
    GetRTDs,
    GetRawRTDs,
    GetPressures,
    GetBMEs,

    None = 0
};

// This map holds all the commands that the Teensy accepts, as str,
// and maps them to an enum for easy access.
const std::unordered_map<TeensyCommands, std::string> cTeensyCommands = {
    /// General system commands
    {TeensyCommands::CheckError,            "CHECKERR"},
    {TeensyCommands::Reset,                 "RESET"},
    /// !General system commands
    ////
    /// Hardware specific commands
    {TeensyCommands::SetPPIDUpdatePeriod,   "SET_PPID_UP"},
    {TeensyCommands::SetPPIDRTD,            "SET_PPID_RTD"},
    {TeensyCommands::SetPPID,               "SET_PPID"},
    {TeensyCommands::SetPPIDTripPoint,      "SET_PTRIPPOINT"},
    {TeensyCommands::SetPPIDTempSetpoint,   "SET_PTEMP"},
    {TeensyCommands::SetPPIDTempKp,         "SET_PTKP_PID"},
    {TeensyCommands::SetPPIDTempTi,         "SET_PTTi_PID"},
    {TeensyCommands::SetPPIDTempTd,         "SET_PTTd_PID"},
    {TeensyCommands::ResetPPID,             "RESET_PPID"},

    {TeensyCommands::SetRTDSamplingPeriod,  "SET_RTD_SP"},
    {TeensyCommands::SetRTDMask,            "RTD_BANK_MASK"},

    {TeensyCommands::SetPeltierRelay,       "SET_PELTIER_RELAY"},

    //// Getters
    {TeensyCommands::GetSystemParameters,   "GET_SYS_PARAMETERS"},
    {TeensyCommands::GetError,              "GETERR"},
    {TeensyCommands::GetPeltiers,           "GET_PELTIERS_CURRS"},
    {TeensyCommands::GetRTDs,               "GET_RTDS"},
    {TeensyCommands::GetRawRTDs,            "GET_RAW_RTDS"},
    {TeensyCommands::GetPressures,          "GET_PRESSURES"},
    {TeensyCommands::GetBMEs,               "GET_BMES"},
    //// !Getters
    /// !Hardware specific commands

    {TeensyCommands::None, ""}
};

struct TeensyControllerData;

// Multi-threading items
using TeensyControllerPipeCallback = std::function<void(TeensyControllerData&)>;

// It accepts any Queue with a FIFO style.
template<template<typename, typename> class QueueType,
         typename Traits,
         typename TokenType>
struct TeensyControllerPipe :
    public Pipe<QueueType, TeensyControllerData, Traits, TokenType> {};

template<class TPipe, PipeEndType Type>
struct TeensyControllerPipeEnd : public PipeEnd<TPipe, Type> {
    explicit TeensyControllerPipeEnd(TPipe p) :
        PipeEnd<TPipe, Type>(p) {}
};

// It holds everything the outside world can modify or use.
// So far, I do not like teensy_serial is here.
struct TeensyControllerData {
    std::string RunDir      = "";

    std::string Port        = "COM4";

    TeensyControllerStates CurrentState = TeensyControllerStates::Standby;
    TeensyCommands CommandToSend = TeensyCommands::None;

    TeensySystemPars SystemParameters;

    uint32_t RTDSamplingPeriod = 100;
    uint32_t RTDMask = 0xFFFF;

    // Relay stuff
    bool PeltierState       = false;

    // PID Stuff
    uint16_t PidRTD = 0;
    uint32_t PeltierPidUpdatePeriod = 100;
    bool PeltierPIDState    = false;

    float PIDTempTripPoint = 5.0;
    PIDConfig PIDTempValues;

    // Indicators
    std::array<double, 9> RTDTemps;

    // Graph data
    PlotDataBuffer<9> TemperatureData;

    // This API required items.
    bool Changed = false;
    TeensyControllerPipeCallback Callback;
};

template<ControlTypes ControlType, StringLiteral Label>
using TeensyControllerControl = Control<ControlType, Label>;

} // namespace SBCQueens

#endif
