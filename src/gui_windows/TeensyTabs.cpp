#include "sbcqueens-gui/gui_windows/TeensyTabs.hpp"

// C STD includes
// C 3rd party includes
// C++ STD includes
// C++ 3rd party includes
// my includes

namespace SBCQueens {

    bool TeensyTabs::operator()() {
        // Actual IMGUI code now
        if (ImGui::BeginTabItem("Teensy")) {
            // This makes all the items between PushItemWidth
            // & PopItemWidth 80 px (?) wide
            ImGui::PushItemWidth(100);

            TeensyControlFac.InputScalar("RTD Sampling Period (ms)",
                tgui_state.RTDSamplingPeriod,
                ImGui::IsItemEdited,
                [=](TeensyControllerData& oldState) {
                        oldState.CommandToSend = TeensyCommands::SetRTDSamplingPeriod;
                        oldState.RTDSamplingPeriod = tgui_state.RTDSamplingPeriod;
                        return true;
                }
            );

            TeensyControlFac.InputScalar("RTD Mask",
                tgui_state.RTDMask,
                ImGui::IsItemEdited,
                [=](TeensyControllerData& oldState) {
                        oldState.CommandToSend = TeensyCommands::SetRTDMask;
                        oldState.RTDMask = tgui_state.RTDMask;
                        return true;
                }
            );

            if (!tgui_state.SystemParameters.InRTDOnlyMode) {
                TeensyControlFac.Checkbox("Peltier Relay",
                    tgui_state.PeltierState,
                    ImGui::IsItemEdited,
                    [=](TeensyControllerData& oldState) {
                        oldState.CommandToSend = TeensyCommands::SetPeltierRelay;
                        oldState.PeltierState = tgui_state.PeltierState;
                        return true;
                    }
                );
            }

            ImGui::PopItemWidth();

            // TeensyControlFac.Checkbox("Pressure Relief Valve",
            //  tgui_state.ReliefValveState,
            //  ImGui::IsItemEdited,
            //  [=](TeensyControllerState& oldState) {
            //      oldState.CommandToSend = TeensyCommands::SetReleaseValve;
            //      oldState.ReliefValveState = tgui_state.ReliefValveState;
            //      return true;
            //  }
            // );

            // TeensyControlFac.Checkbox("N2 Release Relay",
            //  tgui_state.N2ValveState,
            //  ImGui::IsItemEdited,
            //  [=](TeensyControllerState& oldState) {
            //      oldState.CommandToSend = TeensyCommands::SetN2Valve;
            //      oldState.N2ValveState = tgui_state.N2ValveState;
            //      return true;
            //  }
            // );



            if (!tgui_state.SystemParameters.InRTDOnlyMode) {
                ImGui::Separator();
                ImGui::PushItemWidth(180);

                ImGui::Text("Peltier PID");
                TeensyControlFac.Checkbox(
                    "Peltier ON/OFF",
                    tgui_state.PeltierPIDState,
                    ImGui::IsItemEdited,
                    [=](TeensyControllerData& oldState) {
                        oldState.CommandToSend = TeensyCommands::SetPPID;
                        oldState.PeltierPIDState = tgui_state.PeltierPIDState;

                        return true;
                    }
                );

                TeensyControlFac.InputScalar("PID RTD",
                    tgui_state.PidRTD,
                    ImGui::IsItemEdited,
                    [=](TeensyControllerData& oldState) {
                        oldState.CommandToSend = TeensyCommands::SetPPIDRTD;
                        oldState.PidRTD = tgui_state.PidRTD;
                        return true;
                    }
                );

                // TeensyControlFac.ComboBox("PID State",
                //  tgui_state.PIDState,
                //  {{PIDState::Running, "Running"}, {PIDState::Standby, "Standby"}},
                //  ImGui::IsItemDeactivated,
                //  [=](TeensyControllerState& oldState) {

                //      oldState.PIDState = tgui_state.PIDState;
                //      if(oldState.PIDState == PIDState::Standby) {
                //          oldState.CommandToSend = TeensyCommands::StopPID;
                //      } else {
                //          oldState.CommandToSend = TeensyCommands::StartPID;
                //      }

                //      return true;
                //  }
                // );

                TeensyControlFac.InputScalar("Update Period (ms)",
                    tgui_state.PeltierPidUpdatePeriod,
                    ImGui::IsItemDeactivated,
                    [=](TeensyControllerData& oldState) {
                        oldState.CommandToSend = TeensyCommands::SetPPIDUpdatePeriod;
                        oldState.PeltierPidUpdatePeriod = tgui_state.PeltierPidUpdatePeriod;
                        return true;
                    }
                );

                TeensyControlFac.InputFloat("Peltier trip point",
                    tgui_state.PIDTempTripPoint,
                    0.01f, 6.0f, "%.6f °C",
                    ImGui::IsItemDeactivated,
                    [=](TeensyControllerData& oldState) {
                        oldState.CommandToSend = TeensyCommands::SetPPIDTripPoint;
                        oldState.PIDTempTripPoint = tgui_state.PIDTempTripPoint;
                        return true;
                    }
                );

                TeensyControlFac.InputFloat("Peltier T Setpoint",
                    tgui_state.PIDTempValues.SetPoint,
                    0.01f, 6.0f, "%.6f °C",
                    ImGui::IsItemDeactivated,
                    [=](TeensyControllerData& oldState) {
                        oldState.CommandToSend = TeensyCommands::SetPPIDTempSetpoint;
                        oldState.PIDTempValues.SetPoint = tgui_state.PIDTempValues.SetPoint;
                        return true;
                    }
                );

                TeensyControlFac.InputFloat("PKp",
                    tgui_state.PIDTempValues.Kp,
                    0.01f, 6.0f, "%.6f",
                    ImGui::IsItemDeactivated,
                    [=](TeensyControllerData& oldState) {
                        oldState.CommandToSend = TeensyCommands::SetPPIDTempKp;
                        oldState.PIDTempValues.Kp = tgui_state.PIDTempValues.Kp;
                        return true;
                    }
                );

                TeensyControlFac.InputFloat("PTi",
                    tgui_state.PIDTempValues.Ti,
                    0.01f, 6.0f, "%.6f ms", ImGui::IsItemDeactivated,
                    [=](TeensyControllerData& oldState) {
                        oldState.CommandToSend = TeensyCommands::SetPPIDTempTi;
                        oldState.PIDTempValues.Ti = tgui_state.PIDTempValues.Ti;
                        return true;
                    }
                );

                TeensyControlFac.InputFloat("PTd",
                    tgui_state.PIDTempValues.Td,
                    0.01f, 6.0f, "%.6f ms",
                    ImGui::IsItemDeactivated,
                    [=](TeensyControllerData& oldState) {
                        oldState.CommandToSend = TeensyCommands::SetPPIDTempTd;
                        oldState.PIDTempValues.Td = tgui_state.PIDTempValues.Td;
                        return true;
                    }
                );

                TeensyControlFac.Button("Reset PPID",
                    [=](TeensyControllerData& oldState) {

                        oldState.CommandToSend = TeensyCommands::ResetPPID;

                        return true;
                    }
                );
            }

            ImGui::EndTabItem();
        }

        return true;
    }

}  // namespace SBCQueens
