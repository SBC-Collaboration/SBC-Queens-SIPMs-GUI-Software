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

#include "sbcqueens-gui/gui_windows/Window.hpp"

#include "sbcqueens-gui/hardware_helpers/SiPMAcquisitionData.hpp"
#include "sbcqueens-gui/hardware_helpers/TeensyControllerData.hpp"
#include "sbcqueens-gui/hardware_helpers/SlowDAQData.hpp"

namespace SBCQueens {

class IndicatorWindow :
public Window<SiPMAcquisitionData, TeensyControllerData, SlowDAQData> {
    // IndicatorReceiver<IndicatorNames>& _indicator_receiver;

    // CAEN Pipe
    SiPMAcquisitionData& _sipm_doe;

    // Teensy Pipe
    TeensyControllerData& _teensy_doe;

    // Slow DAQ pipe
    SlowDAQData& _slowdaq_doe;

 public:
    IndicatorWindow(
    SiPMAcquisitionData& sipm_data, TeensyControllerData& teensy_data,
    SlowDAQData& slow_data) :
        Window<SiPMAcquisitionData, TeensyControllerData, SlowDAQData>{"Indicators"},
        _sipm_doe{sipm_data},
        _teensy_doe(teensy_data),
        _slowdaq_doe(slow_data)
    { }

    ~IndicatorWindow() {}

    void init_window(const toml::table&);

 private:
    void draw();
};

inline auto make_indicator_window(
    SiPMAcquisitionData& sipm_data, TeensyControllerData& teensy_data,
    SlowDAQData& slow_data) {
    return std::make_unique<IndicatorWindow>(sipm_data, teensy_data, slow_data);
}

}  // namespace SBCQueens
#endif
