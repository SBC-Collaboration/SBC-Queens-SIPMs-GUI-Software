#include "sbcqueens-gui/gui_windows/SiPMControlWindow.hpp"

// C STD includes
// C 3rd party includes
// C++ STD includes
// C++ 3rd party includes
// my includes
#include "sbcqueens-gui/imgui_helpers.hpp"
#include "sbcqueens-gui/gui_windows/ControlList.hpp"

namespace SBCQueens {

void SiPMControlWindow::init_window(const toml::table& tb) {
    auto other_conf = tb["Other"];
    auto file_conf = tb["File"];

    _sipm_data.SiPMOutputName = other_conf["SiPM Default Output Name"].value_or("test");
    _sipm_data.SiPMVoltageSysSupplyEN = false;
    _sipm_data.SiPMVoltageSysPort
        = other_conf["SiPMVoltageSystem"]["Port"].value_or("COM6");
    _sipm_data.SiPMVoltageSysVoltage
        = other_conf["SiPMVoltageSystem"]["InitVoltage"].value_or(0.0);


    _sipm_data.VBDData.DataPulses
        = file_conf["RunWaveforms"].value_or(1000000ull);
    _sipm_data.VBDData.SPEEstimationTotalPulses
        = file_conf["GainWaveforms"].value_or(10000ull);
}

void SiPMControlWindow::draw()  {
    static bool tmp;
    constexpr auto start_meas_routine_btn = get_control<ControlTypes::Button,
            "START##CAEN">(SiPMGUIControls);
    draw_control(start_meas_routine_btn, _sipm_data,
                 tmp, [&](){ return tmp; },
            // Callback when IsItemEdited !
                 [&](SiPMAcquisitionData& doe_twin) {
                     if (doe_twin.CurrentState != SiPMAcquisitionManagerStates::Acquisition) {
                         return;
                     }

                     if (doe_twin.AcquisitionState == SiPMAcquisitionStates::Oscilloscope) {
                         doe_twin.AcquisitionState = SiPMAcquisitionStates::EndlessAcquisition;

//                if (doe_twin.SiPMVoltageSysSupplyEN) {
//                    doe_twin.LatestTemperature = _teensy_data.PIDTempValues.SetPoint;
//                } else {
//                    spdlog::warn("Gain calculation cannot start without "
//                        "enabling the power supply.");
//                }
                     }
                 }
    );

    ImGui::SameLine();

    constexpr auto cancel_meas_routine_btn = get_control<ControlTypes::Button,
            "STOP##CAEN">(SiPMGUIControls);
    draw_control(cancel_meas_routine_btn, _sipm_data,
                 tmp, [&](){ return tmp; },
            // Callback when IsItemEdited !
                 [](SiPMAcquisitionData& doe_twin) {
                     if (doe_twin.AcquisitionState == SiPMAcquisitionStates::EndlessAcquisition) {
                         doe_twin.AcquisitionState = SiPMAcquisitionStates::Oscilloscope;
                     }
                 }
    );
    ImGui::Separator();
    ImGui::PushItemWidth(120);

    constexpr auto sipm_out_filename_it = get_control<ControlTypes::InputText, "SiPM Output File Name">(SiPMGUIControls);
    draw_control(sipm_out_filename_it,
                 _sipm_data,
                 _sipm_data.SiPMOutputName,
                 ImGui::IsItemEdited,
                 // Callback when IsItemEdited !
                 [&](SiPMAcquisitionData& doe_twin) {
                     doe_twin.SiPMOutputName = _sipm_data.SiPMOutputName;
                 });

    constexpr auto sipm_id_it = get_control<ControlTypes::InputInt, "SiPM ID">(SiPMGUIControls);
    draw_control(sipm_id_it,
                 _sipm_data,
                 _sipm_data.SiPMID,
        ImGui::IsItemEdited,
        // Callback when IsItemEdited !
        [&](SiPMAcquisitionData& doe_twin) {
          doe_twin.SiPMID = _sipm_data.SiPMID;
    });

    constexpr auto sipm_cell = get_control<ControlTypes::InputInt,
                                            "SiPM Cell">(SiPMGUIControls);
    draw_control(sipm_cell, _sipm_data,
        _sipm_data.CellNumber, ImGui::IsItemEdited,
        // Callback when IsItemEdited !
        [&](SiPMAcquisitionData& doe_twin) {
          doe_twin.CellNumber = _sipm_data.CellNumber;
    });

    ImGui::Separator();
    constexpr auto ps_enable = get_control<ControlTypes::Checkbox,
                                           "PS Enable">(SiPMGUIControls);
    draw_control(ps_enable, _sipm_data, _sipm_data.SiPMVoltageSysSupplyEN,
        ImGui::IsItemEdited,
        // Callback when IsItemEdited !
        [&](SiPMAcquisitionData& doe_twin) {
            // he expects.
            doe_twin.SiPMVoltageSysSupplyEN = _sipm_data.SiPMVoltageSysSupplyEN;
            if (doe_twin.SiPMVoltageSysVoltage >= 60.0f) {
                doe_twin.SiPMVoltageSysVoltage = 60.0f;
            }
    });

    constexpr auto sipm_voltage = get_control<ControlTypes::InputFloat,
                                              "SiPM Voltage">(SiPMGUIControls);
    draw_control(sipm_voltage, _sipm_data, _sipm_data.SiPMVoltageSysVoltage,
        ImGui::IsItemDeactivated,
        // Callback when IsItemEdited !
        [&](SiPMAcquisitionData& doe_twin) {
            // Ignore the input under BVMode or RunMode
            if (doe_twin.CurrentState
                == SiPMAcquisitionManagerStates::Standby) {
                return;
            }

            doe_twin.SiPMVoltageSysVoltage = _sipm_data.SiPMVoltageSysVoltage;

            if (doe_twin.SiPMVoltageSysVoltage >= 60.0f) {
                doe_twin.SiPMVoltageSysVoltage = 60.0f;
            }

            doe_twin.SiPMVoltageSysChange = true;
    });

    ImGui::Separator();
    //  VBD mode controls
}

} // namespace SBCQueens