#include "GUI/SiPMControlWindow.hpp"
#include "imgui.h"

// C STD includes
// C 3rd party includes
// C++ STD includes
// C++ 3rd party includes
// my includes

namespace SBCQueens {

void SiPMControlWindow::init(const toml::table& tb) {
    _config_table = tb;
}

bool SiPMControlWindow::operator()() {
    if(!ImGui::Begin("SiPM Controls")) {
        ImGui::End();
        return false;
    }


    CAENControlFac.Button("Reset CAEN", [](CAENInterfaceData& state){
        return true;
    });
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Resets the CAEN digitizer with new"
            "values found in the control tabs. NOT IMPLEMENTED YET");
    }

    ImGui::Separator();

    CAENControlFac.Checkbox("PS Enable", cgui_state.PSSupplyEN,
        ImGui::IsItemEdited, [=](CAENInterfaceData& old) {
            old.PSSupplyEN = cgui_state.PSSupplyEN;
            old.PSChange = true;
            return true;
        });

    ImGui::SameLine();

    CAENControlFac.InputFloat("SiPM Voltage", cgui_state.PSVoltage,
        0.01f, 60.00f, "%2.2f V",
        ImGui::IsItemDeactivated, [=](CAENInterfaceData& old) {
            old.PSVoltage = cgui_state.PSVoltage;
            old.PSChange = true;
            return true;
        });

    ImGui::Separator();

    CAENControlFac.Button("Gain Mode",
        [](CAENInterfaceData& state) {
            if (state.CurrentState == CAENInterfaceStates::OscilloscopeMode ||
                state.CurrentState == CAENInterfaceStates::RunMode){
                state.CurrentState = CAENInterfaceStates::StatisticsMode;
            }
            return true;
    });

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Starts measuring pulses and processing "
            "them without saving to file. Intended for diagnostics.");
    }

    CAENControlFac.Button(
        "Start SiPM Data Taking",
        [=](CAENInterfaceData& state) {
        // Only change state if its in a work related
        // state, i.e oscilloscope mode
        if (state.CurrentState == CAENInterfaceStates::OscilloscopeMode ||
            state.CurrentState == CAENInterfaceStates::StatisticsMode) {
            state.CurrentState = CAENInterfaceStates::RunMode;
            state.SiPMParameters = cgui_state.SiPMParameters;
        }
        return true;
    });

    CAENControlFac.Button(
        "Stop SiPM Data Taking",
        [=](CAENInterfaceData& state) {
        if (state.CurrentState == CAENInterfaceStates::RunMode) {
            state.CurrentState = CAENInterfaceStates::OscilloscopeMode;
            state.SiPMParameters = cgui_state.SiPMParameters;
        }
        return true;
    });

    ImGui::End();
    return true;
}

}  // namespace SBCQueens