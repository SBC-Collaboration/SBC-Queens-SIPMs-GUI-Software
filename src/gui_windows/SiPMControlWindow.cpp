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
            old.CurrentState == CAENInterfaceStates::MeasurementRoutineMode) {
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

            // When enabling we also send the current voltage shown
            // in the indicator to make sure the user sees the voltage
            // he expects.
            if (cgui_state.SiPMVoltageSysVoltage >= 60.0f) {
                old.SiPMVoltageSysVoltage = 60.0f;
            }

            old.SiPMVoltageSysVoltage = cgui_state.SiPMVoltageSysVoltage;
            return true;
        });
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Enables or disabled the SiPM power supply.");
    }
    ImGui::PopStyleColor(1);

    CAENControlFac.InputFloat("SiPM Voltage", cgui_state.SiPMVoltageSysVoltage,
        0.01f, 60.00f, "%2.2f V",
        ImGui::IsItemDeactivated, [=](CAENInterfaceData& old) {

            // Ignore the input under BVMode or RunMode
            if (old.CurrentState
                == CAENInterfaceStates::MeasurementRoutineMode) {
                return true;
            }

            if (cgui_state.SiPMVoltageSysVoltage >= 60.0f) {
                old.SiPMVoltageSysVoltage = 60.0f;
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
    CAENControlFac.Button("Run measurement Routine",
        [=](CAENInterfaceData& old) {
            if (old.CurrentState == CAENInterfaceStates::OscilloscopeMode) {
                old.CurrentState = CAENInterfaceStates::MeasurementRoutineMode;

                if (old.SiPMVoltageSysSupplyEN) {
                    old.LatestTemperature = tgui_state.PIDTempValues.SetPoint;
                } else {
                    spdlog::warn("Gain calculation cannot start without "
                        "enabling the power supply.");
                }
            }
            return true;
    });
    ImGui::PopStyleColor(3);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Starts the logic to attempt a VBD calculation, and"
            "then takes the pulse data. \n\n"
            "It disables the ability to change the voltage. "
            "If it fails, it will retry. Cancel by pressing Cancel routine.");
    }
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button,
        static_cast<ImVec4>(ImColor::HSV(0.0f, 0.6f, 0.6f)));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
        static_cast<ImVec4>(ImColor::HSV(0.0f, 0.7f, 0.7f)));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
        static_cast<ImVec4>(ImColor::HSV(0.0f, 0.8f, 0.8f)));
    CAENControlFac.Button("Cancel routine",
        [=](CAENInterfaceData& old) {
            if (old.CurrentState == CAENInterfaceStates::MeasurementRoutineMode) {
                old.CancelMeasurements = true;
            }
            return true;
    });
    ImGui::PopStyleColor(3);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Cancels any ongoing measurement routine.");
    }

    indicatorReceiver.booleanIndicator(IndicatorNames::BREAKDOWN_ROUTINE_ONGOING,
        "Calculating breakdown voltage",
        tmp,
        [=](const double& newVal) -> bool {
                return newVal > 0;
    });
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Shows if the measurements for the breakdown voltage "
            "are ongoing");
    }

    indicatorReceiver.booleanIndicator(IndicatorNames::MEASUREMENT_ROUTINE_ONGOING,
        "Acquiring pulse data",
        tmp,
        [=](const double& newVal) -> bool {
                return newVal > 0;
    });
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Shows if the measurements are ongoing.");
    }

    indicatorReceiver.booleanIndicator(IndicatorNames::FINISHED_ROUTINE,
        "Finished with Cell?",
        tmp,
        [=](const double& newVal) -> bool {
                return newVal > 0;
    });
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Shows if the routine is done.");
    }


    //  end VBD mode controls
    ImGui::Separator();
    //  Data taking controls
    // CAENControlFac.Button(
    //     "Start SiPM Data Taking",
    //     [=](CAENInterfaceData& old) {
    //     // Only change state if its in a work related
    //     // state, i.e oscilloscope mode
    //     if (old.CurrentState == CAENInterfaceStates::OscilloscopeMode ||
    //         old.CurrentState == CAENInterfaceStates::BreakdownVoltageMode) {
    //         old.CurrentState = CAENInterfaceStates::RunMode;
    //     }
    //     return true;
    // });
    // ImGui::SameLine();
    // CAENControlFac.Button("Cancel data taking",
    //     [=](CAENInterfaceData& old) {
    //         if (old.CurrentState == CAENInterfaceStates::RunMode) {
    //             old.CancelMeasurements = true;
    //         }
    //         return true;
    // });
    // if (ImGui::IsItemHovered()) {
    //     ImGui::SetTooltip("Starts a data taking routine which ends until "
    //         "the indicator 'Done data taking?' turns green.");
    // }

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
