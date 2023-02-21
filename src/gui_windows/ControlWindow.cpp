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
    _sipm_doe.CurrentState = SiPMAcquisitionManagerStates::Standby;

    /// CAEN model configs

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