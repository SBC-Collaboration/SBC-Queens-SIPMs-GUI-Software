#ifndef CONTROLLIST_h
#define CONTROLLIST_h
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <array>

// C++ 3rd party includes
// My includes
#include "sbcqueens-gui/hardware_helpers/SiPMAcquisitionData.hpp"
#include "sbcqueens-gui/hardware_helpers/TeensyControllerData.hpp"
#include "sbcqueens-gui/hardware_helpers/SlowDAQData.hpp"

namespace SBCQueens {

// Here you find all the compile available controls.

const std::size_t kNUMCAENCONTROLS = 20;
using SiPMAcquisitionControlsArray = std::array<SiPMAcquisitionControl, kNUMCAENCONTROLS>;
constexpr SiPMAcquisitionControlsArray create_caen_controls() {
    return {
        SiPMAcquisitionControl{"Port", "", "Usually 0 as long as there is no "
                "other CAEN digitizers connected. If there are more, "
                "the port increases as they were connected to the "
                "computer."},
        SiPMAcquisitionControl{"Model", ""},
        SiPMAcquisitionControl{"VME Address", ""},
        SiPMAcquisitionControl{"Keithley COM Port", ""},
        SiPMAcquisitionControl{"Connect##CAEN", ""},
        SiPMAcquisitionControl{"Disconnect##CAEN", ""},
        SiPMAcquisitionControl{"Model", ""},
        SiPMAcquisitionControl{"Max Events Per Read", ""},
        SiPMAcquisitionControl{"Record Length [sp]", ""},
        SiPMAcquisitionControl{"Post-Trigger Buffer [%]", ""},
        SiPMAcquisitionControl{"TRG-IN as Gate", ""},
        SiPMAcquisitionControl{"External Trigger Mode", ""},
        SiPMAcquisitionControl{"Software Trigger Mode", ""},
        SiPMAcquisitionControl{"Trigger Polarity", ""},
        SiPMAcquisitionControl{"I/O Level", ""},
        SiPMAcquisitionControl{"Software Trigger", "", "Forces a trigger in "
                "the digitizer if the feature is enabled"},
        SiPMAcquisitionControl{"Reset CAEN", "", "Resets the CAEN digitizer "
                "with new values found in the control tabs."},
        SiPMAcquisitionControl{"SiPM ID", "", "This is the SiPM ID "
        "as specified."},
        SiPMAcquisitionControl{"SiPM Cell", "", "This is the SiPM cell "
                "being tested.  Be mindful that the number on the "
                "feedthrough corresponds to different SiPMs and their cells."},
        SiPMAcquisitionControl{"PS Enable", "", "Enables or disabled the "
                "SiPM power supply."},
    };
}

constexpr static SiPMAcquisitionControlsArray SiPMControls = create_caen_controls();

/// Teensy
const std::size_t kNUMTEENSYCONTROLS = 3;
using TeensyControllerControlArray = std::array<TeensyControllerControl, kNUMTEENSYCONTROLS>;
constexpr TeensyControllerControlArray create_teensy_controls() {
    return {
        TeensyControllerControl{"Teensy COM Port", ""},
        TeensyControllerControl{"Connect##Teensy", ""},
        TeensyControllerControl{"Disconnect##Teensy", ""}
    };
}

constexpr static TeensyControllerControlArray TeensyControls
    = create_teensy_controls();
/// End Teensy

/// SlowDAQ
const std::size_t kNUMSLOWDAQCONTROLS = 3;
using SlowDAQControlsArray = std::array<SlowDAQControl, kNUMSLOWDAQCONTROLS>;
constexpr SlowDAQControlsArray create_sloqdaq_controls() {
    return {
        SlowDAQControl{"PFEIFFER Port", ""},
        SlowDAQControl{"Connect##SLOWDAQ", ""},
        SlowDAQControl{"Disconnect##SLOWDAQ", ""}
    };
}

constexpr static SlowDAQControlsArray SlowDAQControls
    = create_teensy_controls();
/// End SloqDAQ

} // namespace SBCQueens

#endif