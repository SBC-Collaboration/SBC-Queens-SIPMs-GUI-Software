#ifndef CONTROLWINDOW_H
#define CONTROLWINDOW_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <string>
#include <unordered_map>

// C++ 3rd party includes
#include <imgui.h>
#include <toml.hpp>
#include <misc/cpp/imgui_stdlib.h>


// my includes
#include "sbcqueens-gui/multithreading_helpers/Pipe.hpp"

#include "sbcqueens-gui/imgui_helpers.hpp"

#include "sbcqueens-gui/gui_windows/Window.hpp"
#include "sbcqueens-gui/gui_windows/RunTab.hpp"
#include "sbcqueens-gui/gui_windows/TeensyTab.hpp"
#include "sbcqueens-gui/gui_windows/CAENGeneralConfigTab.hpp"
#include "sbcqueens-gui/gui_windows/OtherSmallTabs.hpp"

#include "sbcqueens-gui/hardware_helpers/SiPMAcquisitionData.hpp"
#include "sbcqueens-gui/hardware_helpers/SlowDAQData.hpp"
#include "sbcqueens-gui/hardware_helpers/TeensyControllerData.hpp"

namespace SBCQueens {

class ControlWindow :
public Window<SiPMAcquisitionData, TeensyControllerData, SlowDAQData> {
    // CAEN Pipe
    SiPMAcquisitionData& _sipm_doe;

    // Teensy Pipe
    TeensyControllerData& _teensy_doe;

    // Slow DAQ pipe
    SlowDAQData& _slowdaq_doe;

 public:
    explicit ControlWindow(
    SiPMAcquisitionData& sipm_data, TeensyControllerData& teensy_data,
    SlowDAQData& slow_data) :
        Window<SiPMAcquisitionData, TeensyControllerData, SlowDAQData>{"Window"},
        _sipm_doe{sipm_data},
        _teensy_doe(teensy_data),
        _slowdaq_doe(slow_data)
    {
        this->_tabs.push_back(make_run_tab(sipm_data, teensy_data, slow_data));
        this->_tabs.push_back(make_teensy_tab(teensy_data));
        this->_tabs.push_back(make_caen_general_config_tab(_sipm_doe));
        this->_tabs.push_back(make_gui_config_tab());
    }

    ~ControlWindow() {}

 private:
    void init_window(const toml::table&);
    void draw();
};

inline auto make_control_window(
    SiPMAcquisitionData& sipm_data, TeensyControllerData& teensy_data,
    SlowDAQData& slow_data) {
    return std::make_unique<ControlWindow>(sipm_data, teensy_data, slow_data);
}

}  // namespace SBCQueens

#endif
