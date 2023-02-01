#ifndef TEENSYTABS_H
#define TEENSYTABS_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
// C++ 3rd party includes
#include <imgui.h>

// my includes
#include "sbcqueens-gui/imgui_helpers.hpp"

#include "sbcqueens-gui/gui_windows/Window.hpp"
#include "sbcqueens-gui/gui_windows/ControlList.hpp"

#include "sbcqueens-gui/hardware_helpers/TeensyControllerData.hpp"

namespace SBCQueens {

template <typename Pipes>
class TeensyTab : public Tab<Pipes> {
    // Teensy Pipe
    using TeensyPipe_type = typename Pipes::TeensyPipe_type;
    TeensyControllerPipeEnd<TeensyPipe_type> _teensy_pipe_end;
    TeensyControllerData& _teensy_doe;

 public:
    explicit TeensyTab(const Pipes& p) :
        Tab<Pipes>{p, "Teensy"},
        _teensy_pipe_end{p.TeensyPipe}, _teensy_doe(_teensy_pipe_end.Doe) {}

    ~TeensyTab() {}

    void init_tab(const toml::table& tb) {
        auto t_conf = tb["Teensy"];
        _teensy_doe.RTDSamplingPeriod = t_conf["RTDSamplingPeriod"].value_or(100Lu);
        _teensy_doe.RTDMask = t_conf["RTDMask"].value_or(0xFFFFLu);
        _teensy_doe.PeltierState = false;

        _teensy_doe.PIDTempTripPoint = t_conf["PeltierTempTripPoint"].value_or(0.0f);
        _teensy_doe.PIDTempValues.SetPoint = t_conf["PeltierTempSetpoint"].value_or(0.0f);
        _teensy_doe.PIDTempValues.Kp = t_conf["PeltierTKp"].value_or(0.0f);
        _teensy_doe.PIDTempValues.Ti = t_conf["PeltierTTi"].value_or(0.0f);
        _teensy_doe.PIDTempValues.Td = t_conf["PeltierTTd"].value_or(0.0f);
    }

    void draw() {
        // This makes all the items between PushItemWidth
        // & PopItemWidth 80 px (?) wide
        ImGui::PushItemWidth(100);

        // TeensyControlFac.InputScalar("RTD Sampling Period (ms)",
        //     _teensy_doe.RTDSamplingPeriod,
        //     ImGui::IsItemEdited,
        //     [=](TeensyControllerData& oldState) {
        //             oldState.CommandToSend = TeensyCommands::SetRTDSamplingPeriod;
        //             oldState.RTDSamplingPeriod = _teensy_doe.RTDSamplingPeriod;
        //             return true;
        //     }
        // );

        // TeensyControlFac.InputScalar("RTD Mask",
        //     _teensy_doe.RTDMask,
        //     ImGui::IsItemEdited,
        //     [=](TeensyControllerData& oldState) {
        //             oldState.CommandToSend = TeensyCommands::SetRTDMask;
        //             oldState.RTDMask = _teensy_doe.RTDMask;
        //             return true;
        //     }
        // );

        // if (!_teensy_doe.SystemParameters.InRTDOnlyMode) {
        //     TeensyControlFac.Checkbox("Peltier Relay",
        //         _teensy_doe.PeltierState,
        //         ImGui::IsItemEdited,
        //         [=](TeensyControllerData& oldState) {
        //             oldState.CommandToSend = TeensyCommands::SetPeltierRelay;
        //             oldState.PeltierState = _teensy_doe.PeltierState;
        //             return true;
        //         }
        //     );
        // }

        ImGui::PopItemWidth();

        // TeensyControlFac.Checkbox("Pressure Relief Valve",
        //  _teensy_doe.ReliefValveState,
        //  ImGui::IsItemEdited,
        //  [=](TeensyControllerState& oldState) {
        //      oldState.CommandToSend = TeensyCommands::SetReleaseValve;
        //      oldState.ReliefValveState = _teensy_doe.ReliefValveState;
        //      return true;
        //  }
        // );

        // TeensyControlFac.Checkbox("N2 Release Relay",
        //  _teensy_doe.N2ValveState,
        //  ImGui::IsItemEdited,
        //  [=](TeensyControllerState& oldState) {
        //      oldState.CommandToSend = TeensyCommands::SetN2Valve;
        //      oldState.N2ValveState = _teensy_doe.N2ValveState;
        //      return true;
        //  }
        // );

        if (!_teensy_doe.SystemParameters.InRTDOnlyMode) {
            ImGui::Separator();
            ImGui::PushItemWidth(180);

            ImGui::Text("Peltier PID");
            // TeensyControlFac.Checkbox(
            //     "Peltier ON/OFF",
            //     _teensy_doe.PeltierPIDState,
            //     ImGui::IsItemEdited,
            //     [=](TeensyControllerData& oldState) {
            //         oldState.CommandToSend = TeensyCommands::SetPPID;
            //         oldState.PeltierPIDState = _teensy_doe.PeltierPIDState;

            //         return true;
            //     }
            // );

            constexpr Control peltier_switch = get_control(SiPMGUIControls, "Peltier ON/OFF");
            draw_control(_teensy_doe, peltier_switch, Checkbox,
                _teensy_doe.PeltierPIDState, ImGui::IsItemEdited,
                // Callback when IsItemEdited !
                [doe = _teensy_doe]
                (TeensyControllerData& teensy_twin) {
                    teensy_twin.CommandToSend = TeensyCommands::SetPPID;
                    teensy_twin.PeltierPIDState = doe.PeltierPIDState;
            });


            // TeensyControlFac.InputScalar("PID RTD",
            //     _teensy_doe.PidRTD,
            //     ImGui::IsItemEdited,
            //     [=](TeensyControllerData& oldState) {
            //         oldState.CommandToSend = TeensyCommands::SetPPIDRTD;
            //         oldState.PidRTD = _teensy_doe.PidRTD;
            //         return true;
            //     }
            // );

            constexpr Control pid_switch = get_control(SiPMGUIControls, "PID RTD");
            draw_control(_teensy_doe, pid_switch, InputUINT16,
                _teensy_doe.PidRTD, ImGui::IsItemEdited,
                // Callback when IsItemEdited !
                [doe = _teensy_doe]
                (TeensyControllerData& teensy_twin) {
                    teensy_twin.CommandToSend = TeensyCommands::SetPPIDRTD;
                    teensy_twin.PidRTD = doe.PidRTD;
            });

            // TeensyControlFac.ComboBox("PID State",
            //  _teensy_doe.PIDState,
            //  {{PIDState::Running, "Running"}, {PIDState::Standby, "Standby"}},
            //  ImGui::IsItemDeactivated,
            //  [=](TeensyControllerState& oldState) {

            //      oldState.PIDState = _teensy_doe.PIDState;
            //      if(oldState.PIDState == PIDState::Standby) {
            //          oldState.CommandToSend = TeensyCommands::StopPID;
            //      } else {
            //          oldState.CommandToSend = TeensyCommands::StartPID;
            //      }

            //      return true;
            //  }
            // );

            constexpr Control update_period = get_control(SiPMGUIControls, "Update Period (ms)");
            draw_control(_teensy_doe, update_period, InputUINT32,
                _teensy_doe.PeltierPidUpdatePeriod, ImGui::IsItemDeactivated,
                // Callback when IsItemEdited !
                [doe = _teensy_doe]
                (TeensyControllerData& teensy_twin) {
                    teensy_twin.CommandToSend = TeensyCommands::SetPPIDUpdatePeriod;
                    teensy_twin.PeltierPidUpdatePeriod = doe.PeltierPidUpdatePeriod;
            });
            // TeensyControlFac.InputScalar("Update Period (ms)",
            //     _teensy_doe.PeltierPidUpdatePeriod,
            //     ImGui::IsItemDeactivated,
            //     [=](TeensyControllerData& oldState) {
            //         oldState.CommandToSend = TeensyCommands::SetPPIDUpdatePeriod;
            //         oldState.PeltierPidUpdatePeriod = _teensy_doe.PeltierPidUpdatePeriod;
            //         return true;
            //     }
            // );

            // TeensyControlFac.InputFloat("Peltier trip point",
            //     _teensy_doe.PIDTempTripPoint,
            //     0.01f, 6.0f, "%.6f °C",
            //     ImGui::IsItemDeactivated,
            //     [=](TeensyControllerData& oldState) {
            //         oldState.CommandToSend = TeensyCommands::SetPPIDTripPoint;
            //         oldState.PIDTempTripPoint = _teensy_doe.PIDTempTripPoint;
            //         return true;
            //     }
            // );

            constexpr Control peltier_trip_p = get_control(SiPMGUIControls, "Peltier Trip Point");
            draw_control(_teensy_doe, peltier_trip_p, InputFloat,
                _teensy_doe.PIDTempTripPoint, ImGui::IsItemDeactivated,
                // Callback when IsItemEdited !
                [doe = _teensy_doe]
                (TeensyControllerData& teensy_twin) {
                    teensy_twin.CommandToSend = TeensyCommands::SetPPIDTripPoint;
                    teensy_twin.PIDTempTripPoint = doe.PIDTempTripPoint;
            });

            // TeensyControlFac.InputFloat("Peltier T Setpoint",
            //     _teensy_doe.PIDTempValues.SetPoint,
            //     0.01f, 6.0f, "%.6f °C",
            //     ImGui::IsItemDeactivated,
            //     [=](TeensyControllerData& oldState) {
            //         oldState.CommandToSend = TeensyCommands::SetPPIDTempSetpoint;
            //         oldState.PIDTempValues.SetPoint = _teensy_doe.PIDTempValues.SetPoint;
            //         return true;
            //     }
            // );
            constexpr Control peltier_t_set = get_control(SiPMGUIControls, "Peltier T Setpoint");
            draw_control(_teensy_doe, peltier_t_set, InputFloat,
                _teensy_doe.PIDTempValues.SetPoint, ImGui::IsItemDeactivated,
                // Callback when IsItemEdited !
                [doe = _teensy_doe]
                (TeensyControllerData& teensy_twin) {
                    teensy_twin.CommandToSend = TeensyCommands::SetPPIDTempSetpoint;
                    teensy_twin.PIDTempValues.SetPoint = doe.PIDTempValues.SetPoint;
            });

            // TeensyControlFac.InputFloat("PKp",
            //     _teensy_doe.PIDTempValues.Kp,
            //     0.01f, 6.0f, "%.6f",
            //     ImGui::IsItemDeactivated,
            //     [=](TeensyControllerData& oldState) {
            //         oldState.CommandToSend = TeensyCommands::SetPPIDTempKp;
            //         oldState.PIDTempValues.Kp = _teensy_doe.PIDTempValues.Kp;
            //         return true;
            //     }
            // );
            constexpr Control pkp = get_control(SiPMGUIControls, "PKp");
            draw_control(_teensy_doe, pkp, InputFloat,
                _teensy_doe.PIDTempValues.Kp, ImGui::IsItemDeactivated,
                // Callback when IsItemEdited !
                [doe = _teensy_doe]
                (TeensyControllerData& teensy_twin) {
                    teensy_twin.CommandToSend = TeensyCommands::SetPPIDTempKp;
                    teensy_twin.PIDTempValues.Kp = doe.PIDTempValues.Kp;
            });

            // TeensyControlFac.InputFloat("PTi",
            //     _teensy_doe.PIDTempValues.Ti,
            //     0.01f, 6.0f, "%.6f ms", ImGui::IsItemDeactivated,
            //     [=](TeensyControllerData& oldState) {
            //         oldState.CommandToSend = TeensyCommands::SetPPIDTempTi;
            //         oldState.PIDTempValues.Ti = _teensy_doe.PIDTempValues.Ti;
            //         return true;
            //     }
            // );
            constexpr Control pti = get_control(SiPMGUIControls, "PTi");
            draw_control(_teensy_doe, pti, InputFloat,
                _teensy_doe.PIDTempValues.Ti, ImGui::IsItemDeactivated,
                // Callback when IsItemEdited !
                [doe = _teensy_doe]
                (TeensyControllerData& teensy_twin) {
                    teensy_twin.CommandToSend = TeensyCommands::SetPPIDTempTi;
                    teensy_twin.PIDTempValues.Ti = doe.PIDTempValues.Ti;
            });

            // TeensyControlFac.InputFloat("PTd",
            //     _teensy_doe.PIDTempValues.Td,
            //     0.01f, 6.0f, "%.6f ms",
            //     ImGui::IsItemDeactivated,
            //     [=](TeensyControllerData& oldState) {
            //         oldState.CommandToSend = TeensyCommands::SetPPIDTempTd;
            //         oldState.PIDTempValues.Td = _teensy_doe.PIDTempValues.Td;
            //         return true;
            //     }
            // );
            constexpr Control ptd = get_control(SiPMGUIControls, "PTd");
            draw_control(_teensy_doe, ptd, InputFloat,
                _teensy_doe.PIDTempValues.Td, ImGui::IsItemDeactivated,
                // Callback when IsItemEdited !
                [doe = _teensy_doe]
                (TeensyControllerData& teensy_twin) {
                    teensy_twin.CommandToSend = TeensyCommands::SetPPIDTempTd;
                    teensy_twin.PIDTempValues.Td = doe.PIDTempValues.Td;
            });

            // TeensyControlFac.Button("Reset PPID",
            //     [=](TeensyControllerData& oldState) {

            //         oldState.CommandToSend = TeensyCommands::ResetPPID;

            //         return true;
            //     }
            // );
            bool tmp;
            constexpr Control reset_ppid = get_control(SiPMGUIControls, "Reset PPID");
            draw_control(_teensy_doe, reset_ppid, Button,
                tmp, [&](){ return tmp; },
                // Callback when IsItemEdited !
                [doe = _teensy_doe]
                (TeensyControllerData& teensy_twin) {
                     teensy_twin.CommandToSend = TeensyCommands::ResetPPID;
            });
        }
    }
};

template<typename Pipes>
auto make_teensy_tab(const Pipes& p) {
    return std::make_unique<TeensyTab<Pipes>>(p);
}

}  // namespace SBCQueens
#endif
