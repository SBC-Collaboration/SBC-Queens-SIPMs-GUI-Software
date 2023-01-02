#ifndef INDICATORWINDOW_H
#define INDICATORWINDOW_H
#pragma once

// C STD includes
// C 3rd party includes
#include <imgui.h>

// C++ STD includes
// C++ 3rd party includes
#include <toml.hpp>

// my includes
#include "sbcqueens-gui/indicators.hpp"

#include "sbcqueens-gui/hardware_helpers/CAENInterfaceData.hpp"
#include "sbcqueens-gui/hardware_helpers/TeensyControllerInterface.hpp"
#include "sbcqueens-gui/hardware_helpers/SlowDAQInterface.hpp"

namespace SBCQueens {

class IndicatorWindow {
    toml::table _config_table;
    IndicatorReceiver<IndicatorNames>& _indicator_receiver;
    CAENQueue& _cq;
    TeensyControllerData& _td;
    SlowDAQData& _sd;

 public:
    IndicatorWindow(IndicatorReceiver<IndicatorNames>& ir,
        CAENQueue& cq, TeensyControllerData& td, SlowDAQData& sd)
        : _indicator_receiver(ir), _cq(cq), _td(td), _sd(sd) {}

    // Moving allowed
    IndicatorWindow(IndicatorWindow&&) = default;

    // No copying
    IndicatorWindow(const IndicatorWindow&) = delete;

    void Init(const toml::table& tb);

    bool operator()();
};

}  // namespace SBCQueens
#endif
