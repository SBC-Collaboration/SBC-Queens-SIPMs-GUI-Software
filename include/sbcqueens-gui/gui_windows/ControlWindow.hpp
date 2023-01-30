#ifndef CONTROLWINDOW_H
#define CONTROLWINDOW_H

// C STD includes
// C 3rd party includes
// C++ std includes
// C++ 3rd party includes
// my includes
#include "sbcqueens-gui/hardware_helpers/SiPMAcquisitionData.hpp"
#include "sbcqueens-gui/hardware_helpers/SlowDAQData.hpp"
#include "sbcqueens-gui/hardware_helpers/TeensyControllerData.hpp"

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

namespace SBCQueens {

template<typename Pipes>
class ControlWindow : public Window<Pipes> {
    // CAEN Pipe
    using SiPMPipe_type = typename Pipes::SiPMPipe_type;
    SiPMAcquisitionPipeEnd<SiPMPipe_type> _sipm_pipe_end;
    SiPMAcquisitionData& _sipm_doe;

    // Teensy Pipe
    using TeensyPipe_type = typename Pipes::TeensyPipe_type;
    TeensyControllerPipeEnd<TeensyPipe_type> _teensy_pipe_end;
    TeensyControllerData& _teensy_doe;

    // Slow DAQ pipe
    using SlowDAQ_type = typename Pipes::SlowDAQ_type;
    SlowDAQPipeEnd<SlowDAQ_type> _slowdaq_pipe_end;
    SlowDAQData& _slowdaq_doe;

 public:

    explicit ControlWindow(const Pipes& p) :
        Window<Pipes>{p, "Window"},
        _sipm_pipe_end{p.SiPMPipe}, _sipm_doe{_sipm_pipe_end.Doe},
        _teensy_pipe_end{p.TeensyPipe}, _teensy_doe(_teensy_pipe_end.Doe),
        _slowdaq_pipe_end{p.SlowDAQPipe}, _slowdaq_doe(_slowdaq_pipe_end.Doe)
    {
        this->_tabs.push_back(make_run_tab(p));
        this->_tabs.push_back(make_teensy_tab(p));
        this->_tabs.push_back(make_caen_general_config_tab(p));
        this->_tabs.push_back(make_gui_config_tab(p));
    }

    ~ControlWindow() {}

 private:
    void init_window(const toml::table&);
    void draw();
};

template<typename Pipes>
void ControlWindow<Pipes>::init_window(const toml::table& tb) {
    auto t_conf = tb["Teensy"];
    auto CAEN_conf = tb["CAEN"];
    auto other_conf = tb["Other"];
    auto file_conf = tb["File"];


    /// Teensy config
    // Teensy initial state
    _teensy_doe.CurrentState = TeensyControllerStates::Standby;

    /// CAEN config
    // CAEN initial state
    _sipm_doe.CurrentState = SiPMAcquisitionStates::Standby;

    /// CAEN model configs

    // We check how many CAEN.groupX there are and create that many
    // groups.
    // const uint8_t c_MAX_CHANNELS = 64;
    // for (uint8_t ch = 0; ch < c_MAX_CHANNELS; ch++) {
    //     std::string ch_toml = "group" + std::to_string(ch);
    //     if (CAEN_conf[ch_toml].as_table()) {
    //         _sipm_doe.GroupConfigs.emplace_back(
    //             CAENGroupConfig{
    //                 ch,  // Number uint8_t
    //                 CAEN_conf[ch_toml]["TrgMask"].value_or(0u),
    //                 // TriggerMask uint8_t
    //                 CAEN_conf[ch_toml]["AcqMask"].value_or(0u),
    //                 // AcquisitionMask uint8_t
    //                 CAEN_conf[ch_toml]["Offset"].value_or(0x8000u),
    //                 // DCOffset uint8_t
    //                 std::vector<uint8_t>(),  // DCCorrections
    //                 // uint8_t
    //                 CAEN_conf[ch_toml]["Range"].value_or(0u),
    //                 // DCRange
    //                 CAEN_conf[ch_toml]["Threshold"].value_or(0x8000u)
    //                 // TriggerThreshold

    //             });

    //         if (toml::array* arr = CAEN_conf[ch_toml]["Corrections"].as_array()) {
    //             spdlog::info("Corrections exist");
    //             size_t j = 0;
    //             CAENGroupConfig& last_config = _sipm_doe.GroupConfigs.back();
    //             for (toml::node& elem : *arr) {
    //                 // Max number of channels per group there can be is 8
    //                 if (j < 8){
    //                     last_config.DCCorrections.emplace_back(
    //                         elem.value_or(0u));
    //                     j++;
    //                 }
    //             }
    //         }
    //     }
    // }

    /// Other PFEIFFERSingleGauge
    _slowdaq_doe.PFEIFFERSingleGaugeEnable
        = other_conf["PFEIFFERSingleGauge"]["Enabled"].value_or(false);
    _slowdaq_doe.PFEIFFERSingleGaugeUpdateSpeed
        = static_cast<PFEIFFERSingleGaugeSP>(
            other_conf["PFEIFFERSingleGauge"]["Enabled"].value_or(0));
}

template<class Pipes>
void ControlWindow<Pipes>::draw() {
    // Nothing here really. Everything is managed by the tabs.
    // But this is an empty canvas! Draw something!
}

template<typename Pipes>
ControlWindow<Pipes> make_control_window(const Pipes& p) {
    return ControlWindow<Pipes>(p);
}

}  // namespace SBCQueens

#endif
