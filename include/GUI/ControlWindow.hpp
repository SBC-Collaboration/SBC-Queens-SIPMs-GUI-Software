#ifndef CONTROLWINDOW_H
#define CONTROLWINDOW_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <string>

// C++ 3rd party includes
#include <imgui.h>
#include <toml.hpp>

// my includes
#include "imgui_helpers.hpp"
#include "../TeensyControllerInterface.hpp"
#include "../CAENDigitizerInterface.hpp"
#include "../SlowDAQInterface.hpp"

#include "TeensyTabs.hpp"
#include "CAENTabs.hpp"

namespace SBCQueens {

class ControlWindow {
    toml::table _config_table;

    // Teensy Controls
    ControlLink<TeensyQueue>& TeensyControlFac;
    TeensyControllerData& tgui_state;

    // CAEN Controls
    ControlLink<CAENQueue>& CAENControlFac;
    CAENInterfaceData& cgui_state;

    //
    ControlLink<SlowDAQQueue>& SlowDAQControlFac;
    SlowDAQData& other_state;

    std::string i_run_dir;
    std::string i_run_name;

    TeensyTabs ttabs;
    CAENTabs ctabs;

 public:
    ControlWindow(ControlLink<TeensyQueue>& tc, TeensyControllerData& ts,
        ControlLink<CAENQueue>& cc, CAENInterfaceData& cd,
        ControlLink<SlowDAQQueue>& oc, SlowDAQData& od) :
         TeensyControlFac(tc), tgui_state(ts),
         CAENControlFac(cc), cgui_state(cd),
         SlowDAQControlFac(oc), other_state(od),
         ttabs(tc, ts), ctabs(cc, cd)
    {}

    // Moving allowed
    ControlWindow(ControlWindow&&) = default;

    // No copying
    ControlWindow(const ControlWindow&) = delete;

    void init(const toml::table& tb);

    bool operator()();

 private:
    void _gui_controls_tab();
};

}  // namespace SBCQueens

#endif
