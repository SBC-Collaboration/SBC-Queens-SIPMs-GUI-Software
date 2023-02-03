#ifndef SIPMCONTROLWINDOW_H
#define SIPMCONTROLWINDOW_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
// C++ 3rd party includes
#include <toml.hpp>

// my includes
#include "sbcqueens-gui/imgui_helpers.hpp"

#include "sbcqueens-gui/gui_windows/Window.hpp"
#include "sbcqueens-gui/gui_windows/ControlList.hpp"

#include "sbcqueens-gui/hardware_helpers/SiPMAcquisitionData.hpp"
#include "sbcqueens-gui/hardware_helpers/TeensyControllerData.hpp"

#include "sbcqueens-gui/indicators.hpp"

namespace SBCQueens {

class SiPMControlWindow :
public Window<SiPMAcquisitionData, TeensyControllerData> {
    SiPMAcquisitionData& _sipm_data;
    TeensyControllerData& _teensy_data;

 public:
    SiPMControlWindow(
    SiPMAcquisitionData& sipm_data, TeensyControllerData& teensy_data) :
        Window<SiPMAcquisitionData, TeensyControllerData>{"SiPM Controls"} ,
        _sipm_data(sipm_data), _teensy_data(teensy_data)
    {}

    ~SiPMControlWindow() {}

    void init_window(const toml::table& tb);

 private:
    void draw();
};

inline auto make_sipm_control_window(
    SiPMAcquisitionData& sipm_data, TeensyControllerData& teensy_data) {
    return std::make_unique<SiPMControlWindow>(sipm_data, teensy_data);
}

}  // namespace SBCQueens

#endif
