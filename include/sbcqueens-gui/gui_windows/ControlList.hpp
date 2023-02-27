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
        "Forces a trigger in the digitizer if the feature is enabled",
        DrawingOptions{
            .Color = MutedGreen,
            .HoveredColor = ActiveMutedGreen,
            .ActiveColor = InactiveMutedGreen,
            .Size = {0, 50}
        }},
    SiPMAcquisitionControl<ControlTypes::Button, "Reset##CAEN">{"",
        "Resets the CAEN digitizer with new values found in the "
        "control tabs.",
        DrawingOptions{
            .Color = HSV(0.62f, 0.71f, 0.86f),
            .HoveredColor = HSV(0.62f, 0.80f, 0.96f),
            .ActiveColor = HSV(0.62f, 0.80f, 0.66f),
            .Size = {75, 50}
        }},

    SiPMAcquisitionControl<ControlTypes::InputText, "SiPM Output File Name">{"",
        "Name of the output file. Saved under {Run File}/{Today date}/{this}"},
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
    SiPMAcquisitionControl<ControlTypes::Button, "START##CAEN">{"",
            "Starts the acquisition and file saving routine.",
            DrawingOptions{
                    .Color = HSV(118.f, 0.4f, 0.5f),
                    .HoveredColor = HSV(118.f, 0.4, 0.7f),
                    .ActiveColor = HSV(118.f, 0.4f, 0.2f),
                    .Size = {100, 50}
            }},
    SiPMAcquisitionControl<ControlTypes::Button, "STOP##CAEN">{"",
            "Cancels any ongoing measurement routine.",
            DrawingOptions{
                   .Color = HSV(0.f, 0.8f, 0.5f),
                   .HoveredColor = HSV(0.f, 0.4, 0.7f),
                   .ActiveColor = HSV(0.f, 0.8f, 0.2f),
                   .Size = {100, 50}
            }},

    // Per Group config controls
    SiPMAcquisitionControl<ControlTypes::InputUINT8, "Group to modify">{"",
        "ID of the group to modify. If the digitizer does not support groups "
        "this is the channel number.",
        DrawingOptions{.StepSize = 1, .Format = "%d"}},
    SiPMAcquisitionControl<ControlTypes::Checkbox, "Group Enable">{"",
            "Enables or disables this group."},

    SiPMAcquisitionControl<ControlTypes::InputUINT32, "DC Offset">{"",
        "",
        DrawingOptions{.StepSize = 1, .Format = "%d"}},
    SiPMAcquisitionControl<ControlTypes::InputUINT16, "DC Correction">{"",
        "",
        DrawingOptions{.StepSize = 1, .Format = "%d"}},
    SiPMAcquisitionControl<ControlTypes::InputUINT32, "Threshold">{"",
        "",
        DrawingOptions{.StepSize = 1, .Format = "%d"}},

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

    SiPMAcquisitionControl<ControlTypes::InputUINT8, "Correction 0">{""},
    SiPMAcquisitionControl<ControlTypes::InputUINT8, "Correction 1">{""},
    SiPMAcquisitionControl<ControlTypes::InputUINT8, "Correction 2">{""},
    SiPMAcquisitionControl<ControlTypes::InputUINT8, "Correction 3">{""},
    SiPMAcquisitionControl<ControlTypes::InputUINT8, "Correction 4">{""},
    SiPMAcquisitionControl<ControlTypes::InputUINT8, "Correction 5">{""},
    SiPMAcquisitionControl<ControlTypes::InputUINT8, "Correction 6">{""},
    SiPMAcquisitionControl<ControlTypes::InputUINT8, "Correction 7">{""},

    SiPMAcquisitionControl<ControlTypes::InputUINT32, "Majority Level">{""},
    SiPMAcquisitionControl<ControlTypes::InputUINT32, "Majority Coincidence Window">{""},

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
