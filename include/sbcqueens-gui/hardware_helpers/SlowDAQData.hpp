#ifndef SLOWDAQINTERFACEDATA_H
#define SLOWDAQINTERFACEDATA_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <string>

// C++ 3rd party includes
// my includes
#include "sbcqueens-gui/multithreading_helpers/Pipe.hpp"

#include "sbcqueens-gui/imgui_helpers.hpp"

namespace SBCQueens {

enum class PFEIFFERSingleGaugeSP {
    SLOW = 2,   // 1 min
    FAST = 1,   // 1s
    FASTER = 0  // 100ms
};

struct PFEIFFERSingleGaugeData {
    double time;
    double Pressure;
};

enum class PFEIFFERSSGState {
    Standby,
    AttemptConnection,
    Connected,
    Disconnected,
    Closing
};

struct SlowDAQData;

// Multi-threading items
using SlowDAQPipeCallback = std::function<void(SlowDAQData&)>;

// It accepts any Queue with a FIFO style.
template<template<typename, typename> class QueueType,
         typename Traits,
         typename TokenType>
struct SlowDAQPipe : public Pipe<QueueType, SlowDAQData, Traits, TokenType> {};

template<class TPipe, PipeEndType Type>
struct SlowDAQPipeEnd : public PipeEnd<TPipe, Type> {
    explicit SlowDAQPipeEnd(TPipe p) : PipeEnd<TPipe, Type>(p) {}
};

struct SlowDAQData {
    std::string RunDir      = "";
    std::string RunName     = "";

    std::string PFEIFFERPort = "";

    PFEIFFERSSGState PFEIFFERState = PFEIFFERSSGState::Standby;
    bool PFEIFFERSingleGaugeEnable = false;
    PFEIFFERSingleGaugeSP PFEIFFERSingleGaugeUpdateSpeed
        = PFEIFFERSingleGaugeSP::SLOW;

    // Indicators
    double Vacuum = 0.0; // mbar

    // Graph data
    PlotDataBuffer<1> PressureData;

    // This API required items.
    bool Changed = false;
    SlowDAQPipeCallback Callback;
};

template<ControlTypes ControlType, StringLiteral Label>
using SlowDAQControl = Control<ControlType, Label>;

} // namespace SBCQueens

#endif
