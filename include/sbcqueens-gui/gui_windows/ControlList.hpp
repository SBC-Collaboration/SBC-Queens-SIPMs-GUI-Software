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
    SiPMAcquisitionControl<ControlTypes::InputInt, "CAEN Port">{"",
            "Usually 0 as long as there is no "
            "other CAEN digitizers connected. If there are more, "
            "the port increases as they were connected to the "
            "computer."},
    SiPMAcquisitionControl<ControlTypes::ComboBox, "Connection Type">{""},
    SiPMAcquisitionControl<ControlTypes::ComboBox, "CAEN Model">{""},
    SiPMAcquisitionControl<ControlTypes::InputUINT32, "VME Address">{"", "",
        DrawingOptions{.StepSize = 1, .Format = "%X"}},
    SiPMAcquisitionControl<ControlTypes::InputText, "Keithley COM Port">{""},
    SiPMAcquisitionControl<ControlTypes::Button, "Connect##CAEN">{"", "",
        DrawingOptions{
            .Color = HSV(118.f, 0.4f, 0.5f),
            .HoveredColor = HSV(118.f, 0.4, 0.7f),
            .ActiveColor = HSV(118.f, 0.4f, 0.2f)
        }},
    SiPMAcquisitionControl<ControlTypes::Button, "Disconnect##CAEN">{"", "",
        DrawingOptions{
            .Color = HSV(0.f, 0.8f, 0.5f),
            .HoveredColor = HSV(0.f, 0.4, 0.7f),
            .ActiveColor = HSV(0.f, 0.8f, 0.2f)
        }},
    SiPMAcquisitionControl<ControlTypes::InputUINT32, "Max Events Per Read">{""},
    SiPMAcquisitionControl<ControlTypes::InputUINT32, "Record Length [sp]">{""},
    SiPMAcquisitionControl<ControlTypes::InputUINT32, "Post-Trigger Buffer [%]">{""},
    SiPMAcquisitionControl<ControlTypes::Checkbox, "TRG-IN as Gate">{""},
    SiPMAcquisitionControl<ControlTypes::ComboBox, "External Trigger Mode">{""},
    SiPMAcquisitionControl<ControlTypes::ComboBox, "Software Trigger Mode">{""},
    SiPMAcquisitionControl<ControlTypes::ComboBox, "Trigger Polarity">{""},
    SiPMAcquisitionControl<ControlTypes::ComboBox, "I/O Level">{""},
    SiPMAcquisitionControl<ControlTypes::Button, "Software Trigger">{"",
        "Forces a trigger in the digitizer if the feature is enabled"},
    SiPMAcquisitionControl<ControlTypes::Button, "Reset CAEN">{"",
        "Resets the CAEN digitizer with new values found in the "
        "control tabs."},
    SiPMAcquisitionControl<ControlTypes::InputInt, "SiPM ID">{"",
        "This is the SiPM ID as specified."},
    SiPMAcquisitionControl<ControlTypes::InputInt, "SiPM Cell">{"",
            "This is the SiPM cell being tested.  Be mindful that the number "
            "on the  feedthrough corresponds to different SiPMs and "
            "their cells."},
    SiPMAcquisitionControl<ControlTypes::Checkbox, "PS Enable">{"",
            "Enables or disabled the SiPM power supply."},
    SiPMAcquisitionControl<ControlTypes::InputFloat, "SiPM Voltage">{"",
            "Max voltage allowed is 60V"},
    SiPMAcquisitionControl<ControlTypes::Button, "Run Measurement Routine">{"",
            "Starts the logic to attempt a VBD calculation, and"
            "then takes the pulse data. \n\n"
            "It disables the ability to change the voltage. "
            "If it fails, it will retry. Cancel by pressing Cancel routine."},
    SiPMAcquisitionControl<ControlTypes::Button, "Cancel Measurement Routine">{"",
            "Cancels any ongoing measurement routine."},

    // Per Group config controls
    SiPMAcquisitionControl<ControlTypes::InputUINT8, "Group to modify">{"",
        "ID of the group to modify. If the digitizer does not support groups "
        "this is the channel number.",
        DrawingOptions{.StepSize = 1, .Format = "%d"}},

    SiPMAcquisitionControl<ControlTypes::InputUINT32, "DC Offset">{"",
        "",
        DrawingOptions{.StepSize = 1, .Format = "%X"}},
    SiPMAcquisitionControl<ControlTypes::InputUINT16, "DC Correction">{"",
        "",
        DrawingOptions{.StepSize = 1, .Format = "%X"}},
    SiPMAcquisitionControl<ControlTypes::InputUINT32, "Threshold">{"",
        "",
        DrawingOptions{.StepSize = 1, .Format = "%X"}},

    SiPMAcquisitionControl<ControlTypes::Checkbox, "TRG0">{""},
    SiPMAcquisitionControl<ControlTypes::Checkbox, "TRG1">{""},
    SiPMAcquisitionControl<ControlTypes::Checkbox, "TRG2">{""},
    SiPMAcquisitionControl<ControlTypes::Checkbox, "TRG3">{""},
    SiPMAcquisitionControl<ControlTypes::Checkbox, "TRG4">{""},
    SiPMAcquisitionControl<ControlTypes::Checkbox, "TRG5">{""},
    SiPMAcquisitionControl<ControlTypes::Checkbox, "TRG6">{""},
    SiPMAcquisitionControl<ControlTypes::Checkbox, "TRG7">{""},

    SiPMAcquisitionControl<ControlTypes::Checkbox, "ACQ0">{""},
    SiPMAcquisitionControl<ControlTypes::Checkbox, "ACQ1">{""},
    SiPMAcquisitionControl<ControlTypes::Checkbox, "ACQ2">{""},
    SiPMAcquisitionControl<ControlTypes::Checkbox, "ACQ3">{""},
    SiPMAcquisitionControl<ControlTypes::Checkbox, "ACQ4">{""},
    SiPMAcquisitionControl<ControlTypes::Checkbox, "ACQ5">{""},
    SiPMAcquisitionControl<ControlTypes::Checkbox, "ACQ6">{""},
    SiPMAcquisitionControl<ControlTypes::Checkbox, "ACQ7">{""},

    TeensyControllerControl<ControlTypes::InputText, "Teensy COM Port">{""},
    TeensyControllerControl<ControlTypes::Button, "Connect##Teensy">{"", "",
        DrawingOptions{
            .Color = HSV(118.f, 0.4f, 0.5f),
            .HoveredColor = HSV(118.f, 0.4, 0.7f),
            .ActiveColor = HSV(118.f, 0.4f, 0.2f)
        }},
    TeensyControllerControl<ControlTypes::Button, "Disconnect##Teensy">{"", "",
        DrawingOptions{
            .Color = HSV(0.f, 0.8f, 0.5f),
            .HoveredColor = HSV(0.f, 0.4, 0.7f),
            .ActiveColor = HSV(0.f, 0.8f, 0.2f)
        }},
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
    SlowDAQControl<ControlTypes::Button, "Connect##SLOWDAQ">{"", "",
        DrawingOptions{
            .Color = HSV(118.f, 0.4f, 0.5f),
            .HoveredColor = HSV(118.f, 0.4, 0.7f),
            .ActiveColor = HSV(118.f, 0.4f, 0.2f)
        }},
    SlowDAQControl<ControlTypes::Button, "Disconnect##SLOWDAQ">{"", "",
        DrawingOptions{
            .Color = HSV(0.f, 0.8f, 0.5f),
            .HoveredColor = HSV(0.f, 0.4, 0.7f),
            .ActiveColor = HSV(0.f, 0.8f, 0.2f)
        }}
);

} // namespace SBCQueens

#endif