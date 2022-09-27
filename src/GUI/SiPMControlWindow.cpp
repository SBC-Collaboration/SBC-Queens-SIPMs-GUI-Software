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
    auto other_conf = _config_table["Other"];

    cgui_state.SiPMVoltageSysSupplyEN = false;
    cgui_state.SiPMVoltageSysPort
        = other_conf["SiPMVoltageSystem"]["Port"].value_or("COM6");
    cgui_state.SiPMVoltageSysVoltage
        = other_conf["SiPMVoltageSystem"]["InitVoltage"].value_or(0.0);
}

bool SiPMControlWindow::operator()() {
    if (!ImGui::Begin("SiPM Controls")) {
        ImGui::End();
        return false;
    }

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.84f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
        ImVec4(204.f / 255.f, 170.f / 255.f, 0.0f, 1.0f));
    CAENControlFac.Button("Reset CAEN", [](CAENInterfaceData& old){
        if(old.CurrentState == CAENInterfaceStates::OscilloscopeMode ||
            old.CurrentState == CAENInterfaceStates::RunMode ||
            old.CurrentState == CAENInterfaceStates::StatisticsMode) {
            // Setting it to AttemptConnection will force it to reset
            old.CurrentState = CAENInterfaceStates::AttemptConnection;
        }
        return true;
    });
    ImGui::PopStyleColor(3);

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Resets the CAEN digitizer with new"
            "values found in the control tabs.");
    }

    ImGui::Separator();

    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
    CAENControlFac.Checkbox("PS Enable", cgui_state.SiPMVoltageSysSupplyEN,
        ImGui::IsItemEdited, [=](CAENInterfaceData& old) {
            old.SiPMVoltageSysSupplyEN = cgui_state.SiPMVoltageSysSupplyEN;
            old.SiPMVoltageSysChange = true;
            return true;
        });
    ImGui::PopStyleColor(1);

    ImGui::SameLine();

    CAENControlFac.InputFloat("SiPM Voltage", cgui_state.SiPMVoltageSysVoltage,
        0.01f, 60.00f, "%2.2f V",
        ImGui::IsItemDeactivated, [=](CAENInterfaceData& old) {
            old.SiPMVoltageSysVoltage = cgui_state.SiPMVoltageSysVoltage;
            old.SiPMVoltageSysChange = true;
            return true;
        });

    ImGui::Separator();

    CAENControlFac.Button("Gain Mode",
        [](CAENInterfaceData& old) {
            if (old.CurrentState == CAENInterfaceStates::OscilloscopeMode ||
                old.CurrentState == CAENInterfaceStates::RunMode){
                old.CurrentState = CAENInterfaceStates::StatisticsMode;
            }
            return true;
    });

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Starts measuring pulses and processing "
            "them without saving to file. Intended for diagnostics.");
    }

    CAENControlFac.Button(
        "Start SiPM Data Taking",
        [=](CAENInterfaceData& old) {
        // Only change state if its in a work related
        // state, i.e oscilloscope mode
        if (old.CurrentState == CAENInterfaceStates::OscilloscopeMode ||
            old.CurrentState == CAENInterfaceStates::StatisticsMode) {
            old.CurrentState = CAENInterfaceStates::RunMode;
            old.SiPMParameters = cgui_state.SiPMParameters;
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
