#ifndef SIPMCONTROLWINDOW_H
#define SIPMCONTROLWINDOW_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
// C++ 3rd party includes
#include <toml.hpp>

// my includes
#include "imgui_helpers.hpp"
#include "../CAENDigitizerInterface.hpp"
#include "indicators.hpp"

namespace SBCQueens {

class SiPMControlWindow {
    toml::table _config_table;

    ControlLink<CAENQueue>& CAENControlFac;
    CAENInterfaceData& cgui_state;
    IndicatorReceiver<IndicatorNames>& indicatorReceiver;
 public:
    SiPMControlWindow(ControlLink<CAENQueue>& cc, CAENInterfaceData& cd,
        IndicatorReceiver<IndicatorNames>& ir)
        : CAENControlFac(cc), cgui_state(cd), indicatorReceiver(ir) {}

    // Moving allowed
    SiPMControlWindow(SiPMControlWindow&&) = default;

    // No copying
    SiPMControlWindow(const SiPMControlWindow&) = delete;

    void init(const toml::table& tb);
    bool operator()();
};

}  // namespace SBCQueens

#endif
