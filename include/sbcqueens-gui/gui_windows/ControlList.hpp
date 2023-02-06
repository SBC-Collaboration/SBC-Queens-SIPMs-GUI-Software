#ifndef CONTROLLIST_h
#define CONTROLLIST_h
#include "sbcqueens-gui/imgui_helpers.hpp"
#include <unordered_map>
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <tuple>

// C++ 3rd party includes
// My includes
#include "sbcqueens-gui/hardware_helpers/SiPMAcquisitionData.hpp"
#include "sbcqueens-gui/hardware_helpers/TeensyControllerData.hpp"
#include "sbcqueens-gui/hardware_helpers/SlowDAQData.hpp"

namespace SBCQueens {

// Here you find all the compile available controls.
constexpr static auto SiPMGUIControls = std::make_tuple(
    SiPMAcquisitionControl<ControlTypes::InputInt, "CAEN Port">{"", "Usually 0 as long as there is no "
            "other CAEN digitizers connected. If there are more, "
            "the port increases as they were connected to the "
            "computer."},
    SiPMAcquisitionControl<ControlTypes::ComboBox, "Connection Type">{""},
    SiPMAcquisitionControl<ControlTypes::ComboBox, "Model">{""},
    SiPMAcquisitionControl<ControlTypes::InputUINT32, "VME Address">{""},
    SiPMAcquisitionControl<ControlTypes::InputText, "Keithley COM Port">{""},
    SiPMAcquisitionControl<ControlTypes::Button, "Connect##CAEN">{""},
    SiPMAcquisitionControl<ControlTypes::Button, "Disconnect##CAEN">{""},
    SiPMAcquisitionControl<ControlTypes::InputUINT32, "Max Events Per Read">{""},
    SiPMAcquisitionControl<ControlTypes::InputUINT32, "Record Length [sp]">{""},
    SiPMAcquisitionControl<ControlTypes::InputUINT32, "Post-Trigger Buffer [%]">{""},
    SiPMAcquisitionControl<ControlTypes::Checkbox, "TRG-IN as Gate">{""},
    SiPMAcquisitionControl<ControlTypes::ComboBox, "External Trigger Mode">{""},
    SiPMAcquisitionControl<ControlTypes::ComboBox, "Software Trigger Mode">{""},
    SiPMAcquisitionControl<ControlTypes::ComboBox, "Trigger Polarity">{""},
    SiPMAcquisitionControl<ControlTypes::ComboBox, "I/O Level">{""},
    SiPMAcquisitionControl<ControlTypes::Button, "Software Trigger">{"", "Forces a trigger in "
            "the digitizer if the feature is enabled"},
    SiPMAcquisitionControl<ControlTypes::Button, "Reset CAEN">{"", "Resets the CAEN digitizer "
            "with new values found in the control tabs."},
    SiPMAcquisitionControl<ControlTypes::InputInt, "SiPM ID">{"", "This is the SiPM ID "
    "as specified."},
    SiPMAcquisitionControl<ControlTypes::InputInt, "SiPM Cell">{"", "This is the SiPM cell "
            "being tested.  Be mindful that the number on the "
            "feedthrough corresponds to different SiPMs and their cells."},
    SiPMAcquisitionControl<ControlTypes::Checkbox, "PS Enable">{"", "Enables or disabled the "
            "SiPM power supply."},
    SiPMAcquisitionControl<ControlTypes::InputFloat, "SiPM Voltage">{"",
            "Max voltage allowed is 60V"},
    SiPMAcquisitionControl<ControlTypes::Button, "Run Measurement Routine">{"",
            "Starts the logic to attempt a VBD calculation, and"
            "then takes the pulse data. \n\n"
            "It disables the ability to change the voltage. "
            "If it fails, it will retry. Cancel by pressing Cancel routine."},
    SiPMAcquisitionControl<ControlTypes::Button, "Cancel Measurement Routine">{"", "Cancels any "
            "ongoing measurement routine."},

    TeensyControllerControl<ControlTypes::InputText, "Teensy COM Port">{""},
    TeensyControllerControl<ControlTypes::Button, "Connect##Teensy">{""},
    TeensyControllerControl<ControlTypes::Button, "Disconnect##Teensy">{""},
    TeensyControllerControl<ControlTypes::Checkbox, "Peltier ON/OFF">{""},
    TeensyControllerControl<ControlTypes::InputUINT16, "PID RTD">{""},
    TeensyControllerControl<ControlTypes::InputUINT32, "Update Period (ms)">{""},
    TeensyControllerControl<ControlTypes::InputFloat, "Peltier Trip Point">{""},
    TeensyControllerControl<ControlTypes::InputFloat, "Peltier T Setpoint">{""},
    TeensyControllerControl<ControlTypes::InputFloat, "PKp">{""},
    TeensyControllerControl<ControlTypes::InputFloat, "PTi">{""},
    TeensyControllerControl<ControlTypes::InputFloat, "PTd">{""},
    TeensyControllerControl<ControlTypes::Button, "Reset PPID">{""},

    SlowDAQControl<ControlTypes::InputText, "PFEIFFER Port">{""},
    SlowDAQControl<ControlTypes::Button, "Connect##SLOWDAQ">{""},
    SlowDAQControl<ControlTypes::Button, "Disconnect##SLOWDAQ">{""}
);

} // namespace SBCQueens

#endif