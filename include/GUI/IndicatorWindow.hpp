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
#include "indicators.hpp"

#include "../TeensyControllerInterface.hpp"
#include "../SlowDAQInterface.hpp"

namespace SBCQueens {

class IndicatorWindow {
    toml::table _config_table;
    IndicatorReceiver<IndicatorNames>& _indicator_receiver;
    TeensyControllerData& _td;
    SlowDAQData& _sd;

 public:
    IndicatorWindow(IndicatorReceiver<IndicatorNames>& ir,
        TeensyControllerData& td, SlowDAQData& sd)
        : _indicator_receiver(ir), _td(td), _sd(sd) {}

    // Moving allowed
    IndicatorWindow(IndicatorWindow&&) = default;

    // No copying
    IndicatorWindow(const IndicatorWindow&) = delete;

    void Init(const toml::table& tb);

    bool operator()();
};

}  // namespace SBCQueens
#endif
