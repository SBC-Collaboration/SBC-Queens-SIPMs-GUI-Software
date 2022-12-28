#include "sbcqueens-gui/gui_windows/SiPMControlWindow.hpp"

// C STD includes
// C 3rd party includes
// C++ STD includes
// C++ 3rd party includes
// my includes
#include "sbcqueens-gui/hardware_helpers/Calibration.hpp"
#include <imgui.h>

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
    CAENControlFac.Button("Reset CAEN", [&](CAENInterfaceData& old){
        if (old.CurrentState == CAENInterfaceStates::OscilloscopeMode ||
            old.CurrentState == CAENInterfaceStates::RunMode ||
            old.CurrentState == CAENInterfaceStates::BreakdownVoltageMode) {
            // Setting it to AttemptConnection will force it to reset
            old = cgui_state;
            old.SiPMVoltageSysChange = false;
            old.CurrentState = CAENInterfaceStates::AttemptConnection;
        }
        return true;
    });
    ImGui::PopStyleColor(3);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Resets the CAEN digitizer with new "
            "values found in the control tabs.");
    }

    ImGui::Separator();
    ImGui::PushItemWidth(120);

    // ImGui::InputInt("SiPM Cell #", &cgui_state.CellNumber);
    CAENControlFac.InputScalar("SiPM ID", cgui_state.SiPMID,
        ImGui::IsItemDeactivated, [=](CAENInterfaceData& old){
            old.SiPMID = cgui_state.SiPMID;
            return true;
    });
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("This is the SiPM ID as specified.");
    }

    CAENControlFac.InputScalar("SiPM Cell", cgui_state.CellNumber,
        ImGui::IsItemDeactivated, [=](CAENInterfaceData& old){
            old.CellNumber = cgui_state.CellNumber;
            return true;
    });
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("This is the SiPM cell being tested. "
            "Be mindful that the number on the feedthrough corresponds to "
            "different SiPMs and their cells.");
    }

    ImGui::Separator();

    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
    CAENControlFac.Checkbox("PS Enable", cgui_state.SiPMVoltageSysSupplyEN,
        ImGui::IsItemEdited, [=](CAENInterfaceData& old) {
            old.SiPMVoltageSysSupplyEN = cgui_state.SiPMVoltageSysSupplyEN;
            old.SiPMVoltageSysChange = true;
            return true;
        });
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Enables or disabled the SiPM power supply.");
    }
    ImGui::PopStyleColor(1);

    CAENControlFac.InputFloat("SiPM Voltage", cgui_state.SiPMVoltageSysVoltage,
        0.01f, 60.00f, "%2.2f V",
        ImGui::IsItemDeactivated, [=](CAENInterfaceData& old) {
            if (cgui_state.SiPMVoltageSysVoltage >= 60.0f) {
                old.SiPMVoltageSysVoltage = 60.0f;
                cgui_state.SiPMVoltageSysVoltage = 60.0f;
            }

            // A change in voltage while in VBD mode should trip a reset
            if (old.CurrentState == CAENInterfaceStates::BreakdownVoltageMode) {
                old.VBDData.State = VBRState::Reset;
            }

            old.SiPMVoltageSysVoltage = cgui_state.SiPMVoltageSysVoltage;
            old.SiPMVoltageSysChange = true;
            return true;
        });

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Max voltage allowed is 60V");
    }

    bool tmp;
    indicatorReceiver.booleanIndicator(IndicatorNames::CURRENT_STABILIZED,
        "Current stabilized?",
        tmp,
        [=](const double& newVal) -> bool {
            return newVal > 0;
        }
    );

    ImGui::Separator();
    //  VBD mode controls
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.60f, 0.6f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
        ImVec4(.0f, 8.f, .8f, 1.0f));
    CAENControlFac.Button("Calculate 1SPE gain",
        [=](CAENInterfaceData& old) {
            if (old.CurrentState == CAENInterfaceStates::OscilloscopeMode ||
                old.CurrentState == CAENInterfaceStates::RunMode ||
                old.CurrentState == CAENInterfaceStates::BreakdownVoltageMode) {
                old.CurrentState = CAENInterfaceStates::BreakdownVoltageMode;
                if (old.SiPMVoltageSysSupplyEN) {
                    old.LatestTemperature = tgui_state.PIDTempValues.SetPoint;
                    old.VBDData.State = VBRState::Init;
                } else {
                    spdlog::warn("Gain calculation cannot start without "
                        "enabling the power supply.");
                }
            }
            return true;
    });
    ImGui::PopStyleColor(3);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Starts the logic to attempt a VBD calculation."
            "Estimates the SPE parameters: A1pe, and pulse "
            "nuisance parameters, for example: rise and fall times."
            "Then it grabs another sample to calculate the gain");
    }

    indicatorReceiver.booleanIndicator(IndicatorNames::ANALYSIS_ONGOING,
        "Processing...",
        tmp,
        [=](const double& newVal) -> bool {
                return newVal > 0;
    });
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Shows the calculations are ongoing");
    }

    indicatorReceiver.booleanIndicator(IndicatorNames::FULL_ANALYSIS_DONE,
        "Done?",
        tmp,
        [=](const double& newVal) -> bool {
                return newVal > 0;
    });
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Shows the calculations finished.");
    }

    CAENControlFac.Button("Calculate VBD",
        [](CAENInterfaceData& old) {
            if (old.CurrentState == CAENInterfaceStates::BreakdownVoltageMode) {
                if(old.VBDData.State == VBRState::Idle) {
                    old.VBDData.State = VBRState::CalculateBreakdownVoltage;
                }
            }
            return true;
    });
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Given enough data points (3 or more) "
            "in the G vs V plot the user can calculate the gain by pressing "
            "this button.");
    }

    ImGui::SameLine();
    indicatorReceiver.booleanIndicator(IndicatorNames::CALCULATING_VBD,
        "Done?",
        tmp,
        [=](const double& newVal) -> bool {
                return newVal < 0;
    });
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Shows if the breakdown voltage calculations "
            "have finalized.");
    }

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
        ImVec4(0.8f, .1f, .1f, 1.0f));
    CAENControlFac.Button("Reset VBD",
        [](CAENInterfaceData& old) {
            if (old.CurrentState == CAENInterfaceStates::BreakdownVoltageMode) {
                old.VBDData.State = VBRState::ResetAll;
            }
            return true;
    });
    ImGui::PopStyleColor(2);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Resets the internal buffer for the breakdown buffer "
            "calculations");
    }
    ImGui::SameLine();
    indicatorReceiver.booleanIndicator(IndicatorNames::VBD_IN_MEMORY,
        "VBD in memory?",
        tmp,
        [=](const double& newVal) -> bool {
                return newVal < 0;
    });

    //  end VBD mode controls
    ImGui::Separator();
    //  Data taking controls
    CAENControlFac.Button(
        "Start SiPM Data Taking",
        [=](CAENInterfaceData& old) {
        // Only change state if its in a work related
        // state, i.e oscilloscope mode
        if (old.CurrentState == CAENInterfaceStates::OscilloscopeMode ||
            old.CurrentState == CAENInterfaceStates::BreakdownVoltageMode) {
            old.CurrentState = CAENInterfaceStates::RunMode;
        }
        return true;
    });
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Starts a data taking routine which ends until "
            "the indicator 'Done data taking?' turns green.");
    }

    // CAENControlFac.Button(
    //     "Stop SiPM Data Taking",
    //     [=](CAENInterfaceData& state) {
    //     if (state.CurrentState == CAENInterfaceStates::RunMode) {
    //         state.CurrentState = CAENInterfaceStates::OscilloscopeMode;
    //         state.SiPMParameters = cgui_state.SiPMParameters;
    //     }
    //     return true;
    // });

    //  end Data taking controls
    ImGui::End();
    return true;
}

}  // namespace SBCQueens
