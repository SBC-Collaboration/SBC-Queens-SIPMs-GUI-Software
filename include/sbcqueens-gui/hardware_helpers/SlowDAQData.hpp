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
    NullState = 0,
    Standby,
    AttemptConnection,
    Connected,
    Disconnected,
    Closing
};

struct SlowDAQData;

// Multi-threading items
using SlowDAQPipeCallback = PipeCallback<SlowDAQData>;

// It accepts any Queue with a FIFO style.
template<template<typename, typename> class QueueType, typename Traits>
class SlowDAQPipe : public Pipe<QueueType, SlowDAQData, Traits> {};

template<class TPipe>
class SlowDAQPipeEnd : public PipeEnd<TPipe> {};

struct SlowDAQData {
    std::string RunDir      = "";
    std::string RunName     = "";

    std::string PFEIFFERPort = "";

    PFEIFFERSSGState PFEIFFERState = PFEIFFERSSGState::NullState;
    bool PFEIFFERSingleGaugeEnable = false;
    PFEIFFERSingleGaugeSP PFEIFFERSingleGaugeUpdateSpeed
        = PFEIFFERSingleGaugeSP::SLOW;

    // This API required items.
    bool Changed = false;
    SlowDAQPipeCallback Callback;
};

using SlowDAQControl = Control;

} // namespace SBCQueens

#endif
