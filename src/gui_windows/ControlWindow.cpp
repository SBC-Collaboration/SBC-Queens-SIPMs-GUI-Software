#include "sbcqueens-gui/gui_windows/ControlWindow.hpp"

// C STD includes
// C 3rd party includes
// C++ STD includes
// C++ 3rd party includes
// my includes

namespace SBCQueens {

void ControlWindow::init_window(const toml::table& tb) {
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

void ControlWindow::draw() {
    // Nothing here really. Everything is managed by the tabs.
    // But this is an empty canvas! Draw something!
}

} // namespace SBCQueens