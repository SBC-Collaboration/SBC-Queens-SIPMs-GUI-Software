#ifndef CONTROLWINDOW_H
#define CONTROLWINDOW_H

// C STD includes
// C 3rd party includes
// C++ std includes
// C++ 3rd party includes
// my includes
#include "sbcqueens-gui/hardware_helpers/SiPMAcquisitionData.hpp"
#include "sbcqueens-gui/hardware_helpers/SlowDAQData.hpp"
#include "sbcqueens-gui/hardware_helpers/TeensyControllerData.hpp"

#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <string>
#include <unordered_map>

// C++ 3rd party includes
#include <imgui.h>
#include <toml.hpp>
#include <misc/cpp/imgui_stdlib.h>


// my includes
#include "sbcqueens-gui/multithreading_helpers/Pipe.hpp"

#include "sbcqueens-gui/imgui_helpers.hpp"

#include "sbcqueens-gui/gui_windows/Window.hpp"
#include "sbcqueens-gui/gui_windows/ControlList.hpp"
#include "sbcqueens-gui/gui_windows/TeensyTabs.hpp"
#include "sbcqueens-gui/gui_windows/CAENTabs.hpp"

namespace SBCQueens {

template<typename Pipes>
class ControlWindow : public Window<Pipes> {
    // CAEN Pipe
    using SiPMPipe_type = typename Pipes::SiPMPipe_type;
    SiPMPipe_type _sipm_pipe_end;
    SiPMAcquisitionData& _sipm_doe;

    // Teensy Pipe
    using TeensyPipe_type = typename Pipes::TeensyPipe_type;
    TeensyPipe_type _teensy_pipe_end;
    TeensyControllerData& _teensy_doe;

    // Slow DAQ pipe
    using SlowDAQ_type = typename Pipes::SlowDAQ_type;
    SlowDAQ_type _slowdaq_pipe_end;
    SlowDAQData& _slowdaq_doe;

    std::string i_run_dir;
    std::string i_run_name;

    TeensyTabs ttabs;
    CAENTabs ctabs;

 public:

    explicit ControlWindow(const Pipes& p) :
         Window<Pipes>{p},
        _sipm_pipe_end{p.SiPMPipe}, _sipm_doe{_sipm_pipe_end.Doe},
        _teensy_pipe_end{p.TeensyPipe}, _teensy_doe{_teensy_pipe_end.Doe},
        _slowdaq_pipe_end{p.SlowDAQPipe}, _slowdaq_doe{_slowdaq_pipe_end.Doe}
    {}

    ~ControlWindow() {}

    template<typename List>
    void init(const List& tb);

 private:
    void draw();
    void _gui_controls_tab();
};

template<typename Pipes>
template<typename List>
void ControlWindow<Pipes>::init(const List& tb) {
    auto t_conf = tb["Teensy"];
    auto CAEN_conf = tb["CAEN"];
    auto other_conf = tb["Other"];
    auto file_conf = tb["File"];

    // These two guys are shared between CAEN and Teensy
    i_run_dir  = file_conf["RunDir"].value_or("./RUNS");
    _sipm_doe.VBDData.DataPulses
        = file_conf["RunWaveforms"].value_or(1000000ull);
    _sipm_doe.VBDData.SPEEstimationTotalPulses
        = file_conf["GainWaveforms"].value_or(10000ull);

    /// Teensy config
    // Teensy initial state
    _teensy_doe.CurrentState = TeensyControllerStates::Standby;
    _teensy_doe.Port         = t_conf["Port"].value_or("COM3");
    _teensy_doe.RunDir       = i_run_dir;

    /// Teensy RTD config
    _teensy_doe.RTDSamplingPeriod
        = t_conf["RTDSamplingPeriod"].value_or(100Lu);
    _teensy_doe.RTDMask
        = t_conf["RTDMask"].value_or(0xFFFFLu);

    /// Teensy PID config
    // Send initial states of the PIDs
    _teensy_doe.PeltierState = false;

    _teensy_doe.PIDTempTripPoint
        = t_conf["PeltierTempTripPoint"].value_or(0.0f);
    _teensy_doe.PIDTempValues.SetPoint
        = t_conf["PeltierTempSetpoint"].value_or(0.0f);
    _teensy_doe.PIDTempValues.Kp
        = t_conf["PeltierTKp"].value_or(0.0f);
    _teensy_doe.PIDTempValues.Ti
        = t_conf["PeltierTTi"].value_or(0.0f);
    _teensy_doe.PIDTempValues.Td
        = t_conf["PeltierTTd"].value_or(0.0f);

    /// CAEN config
    // CAEN initial state
    _sipm_doe.CurrentState = SiPMAcquisitionStates::Standby;
    _sipm_doe.RunDir = i_run_dir;

    /// CAEN model configs
    std::unordered_map<std::string, CAENConnectionType> connection_type_map =
      {{"USB", CAENConnectionType::USB, },
        {"A4818", CAENConnectionType::A4818}};
    _sipm_doe.Model
        = CAENDigitizerModelsMap.at(CAEN_conf["Model"].value_or("DT5730B"));
    _sipm_doe.PortNum
        = CAEN_conf["Port"].value_or(0u);
    _sipm_doe.ConnectionType
      = connection_type_map[CAEN_conf["ConnectionType"].value_or("USB")];
    _sipm_doe.VMEAddress
        = CAEN_conf["VMEAddress"].value_or(0u);
    _sipm_doe.GlobalConfig.MaxEventsPerRead
        = CAEN_conf["MaxEventsPerRead"].value_or(512Lu);
    _sipm_doe.GlobalConfig.RecordLength
        = CAEN_conf["RecordLength"].value_or(2048Lu);
    _sipm_doe.GlobalConfig.PostTriggerPorcentage
        = CAEN_conf["PostBufferPorcentage"].value_or(50u);
    _sipm_doe.GlobalConfig.TriggerOverlappingEn
        = CAEN_conf["OverlappingRejection"].value_or(false);
    _sipm_doe.GlobalConfig.EXTasGate
        = CAEN_conf["TRGINasGate"].value_or(false);
    _sipm_doe.GlobalConfig.EXTTriggerMode
        = static_cast<CAEN_DGTZ_TriggerMode_t>(CAEN_conf["ExternalTrigger"].value_or(0L));
    _sipm_doe.GlobalConfig.SWTriggerMode
        = static_cast<CAEN_DGTZ_TriggerMode_t>(CAEN_conf["SoftwareTrigger"].value_or(0L));
    _sipm_doe.GlobalConfig.TriggerPolarity
        = static_cast<CAEN_DGTZ_TriggerPolarity_t>(CAEN_conf["Polarity"].value_or(0L));
    _sipm_doe.GlobalConfig.IOLevel
        = static_cast<CAEN_DGTZ_IOLevel_t>(CAEN_conf["IOLevel"].value_or(0));
    // We check how many CAEN.groupX there are and create that many
    // groups.
    const uint8_t c_MAX_CHANNELS = 64;
    for (uint8_t ch = 0; ch < c_MAX_CHANNELS; ch++) {
        std::string ch_toml = "group" + std::to_string(ch);
        if (CAEN_conf[ch_toml].as_table()) {
            _sipm_doe.GroupConfigs.emplace_back(
                CAENGroupConfig{
                    ch,  // Number uint8_t
                    CAEN_conf[ch_toml]["TrgMask"].value_or(0u),
                    // TriggerMask uint8_t
                    CAEN_conf[ch_toml]["AcqMask"].value_or(0u),
                    // AcquisitionMask uint8_t
                    CAEN_conf[ch_toml]["Offset"].value_or(0x8000u),
                    // DCOffset uint8_t
                    std::vector<uint8_t>(),  // DCCorrections
                    // uint8_t
                    CAEN_conf[ch_toml]["Range"].value_or(0u),
                    // DCRange
                    CAEN_conf[ch_toml]["Threshold"].value_or(0x8000u)
                    // TriggerThreshold

                });

            if (toml::array* arr = CAEN_conf[ch_toml]["Corrections"].as_array()) {
                spdlog::info("Corrections exist");
                size_t j = 0;
                CAENGroupConfig& last_config = _sipm_doe.GroupConfigs.back();
                for (toml::node& elem : *arr) {
                    // Max number of channels per group there can be is 8
                    if (j < 8){
                        last_config.DCCorrections.emplace_back(
                            elem.value_or(0u));
                        j++;
                    }
                }
            }
        }
    }

    /// Other config
    _slowdaq_doe.RunDir = i_run_dir;

    /// Other PFEIFFERSingleGauge
    _slowdaq_doe.PFEIFFERPort
      = other_conf["PFEIFFERSingleGauge"]["Port"].value_or("COM5");
    _slowdaq_doe.PFEIFFERSingleGaugeEnable
        = other_conf["PFEIFFERSingleGauge"]["Enabled"].value_or(false);
    _slowdaq_doe.PFEIFFERSingleGaugeUpdateSpeed
        = static_cast<PFEIFFERSingleGaugeSP>(
            other_conf["PFEIFFERSingleGauge"]["Enabled"].value_or(0));
}

template<class Pipes>
void ControlWindow<Pipes>::draw() {
    if (not ImGui::Begin("Control Window")) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("ControlTabs")) {
        if (ImGui::BeginTabItem("Run")) {
            ImGui::InputText("Runs Directory", &i_run_dir);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Directory of where all the run "
                    "files are going to be saved at.\n"
                    "Fixed when the connect buttons are pressed.");
            }

            ImGui::Separator();
            ImGui::PushItemWidth(100);
            // 1.0 -> 0.5
            // 0.5 -> 1.0
            // (1.0 - 0.5) / (0.5 - 1.0) = -1
            static float connected_mod = 1.5f;
            // Com port text input
            // We do not make this a GUI queue item because
            // we only need to let the client know when we click connnect
            ImGui::InputText("Teensy COM port", &_teensy_doe.Port);

            ImGui::SameLine(300);
            // Colors to pop up or shadow it depending on the conditions
            ImGui::PushStyleColor(ImGuiCol_Button,
                static_cast<ImVec4>(ImColor::HSV(2.0f / 7.0f, 0.6f, connected_mod*0.6f)));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                static_cast<ImVec4>(ImColor::HSV(2.0f / 7.0f, 0.7f, connected_mod*0.7f)));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                static_cast<ImVec4>(ImColor::HSV(2.0f / 7.0f, 0.8f, connected_mod*0.8f)));
            // The operator () carries the ImGUI drawing functions
            // and the task it sends to its associated queue/thread
            // is the lambda (callback) we pass
            //
            // The double ## and the name is necessary if several
            // controls share a same name in the same context/scope
            // otherwise, ImGUI will not differentiate between them
            // and causes issues.
            // if (TeensyControlFac.Button("Connect##Teensy",
            //     // To make things secure, we pass everything by value
            //     [=](TeensyControllerData& oldState) {
            //         // We copy the local state.
            //         oldState = _teensy_doe;
            //         oldState.RunDir = i_run_dir;
            //         // Except for the fact we are changing the state
            //         oldState.CurrentState
            //             = TeensyControllerStates::AttemptConnection;
            //         return true;
            //     }
            // // This is code that is run locally to this thread
            // // when the button is pressed
            // )) {
            //     connected_mod = 0.5;
            // }
            bool tmp;
            draw_at_spot(_teensy_doe, TeensyControls, "Connect##Teensy",
                tmp, Button, [&](){ return tmp; },
                // Callback when IsItemEdited !
                [doe = _teensy_doe, run_dir = i_run_dir]
                (TeensyControllerData& teensy_twin) {
                    teensy_twin.RunDir = run_dir;
                    teensy_twin.Port = doe.Port;
                    teensy_twin.CurrentState
                        = TeensyControllerStates::AttemptConnection;
            });

            ImGui::PopStyleColor(3);

            /// Disconnect button
            float disconnected_mod = 1.5f - connected_mod;
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button,
                static_cast<ImVec4>(ImColor::HSV(0.0f, 0.6f, disconnected_mod*0.6f)));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                static_cast<ImVec4>(ImColor::HSV(0.0f, 0.7f, disconnected_mod*0.7f)));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                static_cast<ImVec4>(ImColor::HSV(0.0f, 0.8f, disconnected_mod*0.8f)));

            // if (TeensyControlFac.Button("Disconnect##teensy",
            //     [=](TeensyControllerData& oldState) {
            //         // No need to disconnect if its not connected
            //         if (oldState.CurrentState == TeensyControllerStates::Connected) {
            //             oldState.CurrentState
            //                 = TeensyControllerStates::Disconnected;
            //         }
            //         return true;
            //     }
            // )) {
            //     connected_mod = 1.5f;
            // }
            draw_at_spot(_teensy_doe, TeensyControls, "Disconnect##Teensy",
                tmp, InputInt, [&](){ return tmp; },
                // Callback when IsItemEdited !
                [doe = _teensy_doe, run_dir = i_run_dir]
                (TeensyControllerData& teensy_twin) {
                    if (teensy_twin.CurrentState == TeensyControllerStates::Connected) {
                        teensy_twin.CurrentState
                            = TeensyControllerStates::Disconnected;
                    }
            });


            ImGui::PopStyleColor(3);
            ImGui::Separator();

            static float c_connected_mod = 1.5f;
            //ImGui::InputInt("CAEN port", , cgui_state.PortNum);
            draw_at_spot(_sipm_doe, SiPMControls, "CAEN port",
                _sipm_doe.PortNum, InputInt, ImGui::IsItemEdited,
                // Callback when IsItemEdited !
                [doe = _sipm_doe](SiPMAcquisitionData& caen_twin) {
                  caen_twin.PortNum = doe.PortNum;
            });

            ImGui::SameLine();

            // CAENControlFac.ComboBox("Connection Type",
            //     cgui_state.ConnectionType,
            //     {{CAENConnectionType::USB, "USB"},
            //     {CAENConnectionType::A4818, "A4818 USB 3.0 to CONET Adapter"}},
            //     []() {return false;}, [](){});

            draw_at_spot(_sipm_doe, SiPMControls, "CAEN port",
                _sipm_doe.ConnectionType, ComboBox, ImGui::IsItemEdited,
                // Callback when IsItemEdited !
                [doe = _sipm_doe](SiPMAcquisitionData& caen_twin) {
                  caen_twin.ConnectionType = doe.ConnectionType;
                },
                {{CAENConnectionType::USB, "USB"},
                {CAENConnectionType::A4818, "A4818 USB 3.0 to CONET Adapter"}});

            if (_sipm_doe.ConnectionType == CAENConnectionType::A4818) {
                ImGui::SameLine();
                // ImGui::InputScalar("VME Address", ImGuiDataType_U32,
                //     &cgui_state.VMEAddress);
                draw_at_spot(_sipm_doe, SiPMControls, "Model",
                    _sipm_doe.VMEAddress, InputUINT32, ImGui::IsItemEdited,
                    // Callback when IsItemEdited !
                    [doe = _sipm_doe](SiPMAcquisitionData& caen_twin) {
                      caen_twin.VMEAddress = doe.VMEAddress;
                });

            }

            // ImGui::InputText("Keithley COM Port",
            //     &cgui_state.SiPMVoltageSysPort);
            draw_at_spot(_sipm_doe, SiPMControls, "Keithley COM Port",
                _sipm_doe.PortNum, InputInt, ImGui::IsItemEdited,
                // Callback when IsItemEdited !
                [doe = _sipm_doe](SiPMAcquisitionData& caen_twin) {
                  caen_twin.SiPMVoltageSysPort = doe.SiPMVoltageSysPort;
            });

            // ImGui::SameLine(300);
            // Colors to pop up or shadow it depending on the conditions
            ImGui::PushStyleColor(ImGuiCol_Button,
                static_cast<ImVec4>(ImColor::HSV(2.0f / 7.0f, 0.6f, c_connected_mod*0.6f)));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                static_cast<ImVec4>(ImColor::HSV(2.0f / 7.0f, 0.7f, c_connected_mod*0.7f)));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                static_cast<ImVec4>(ImColor::HSV(2.0f / 7.0f, 0.8f, c_connected_mod*0.8f)));

            // This button starts the CAEN communication and sends all
            // the setup configuration
            draw_at_spot(_sipm_doe, SiPMControls, "Connect##CAEN",
                tmp, Button, [&](){ return tmp; },
                // Callback when tmp is true !
                [doe = _sipm_doe, run_dir = i_run_dir]
                (SiPMAcquisitionData& caen_twin) {
                    caen_twin = doe;

                    caen_twin.RunDir = run_dir;
                    caen_twin.CurrentState =
                     SiPMAcquisitionStates::AttemptConnection;
            });
            // if (CAENControlFac.Button("Connect##caen",
            //     [=](SiPMAcquisitionData& state) {
            //         state = cgui_state;
            //         state.VBDData = cgui_state.VBDData;
            //         state.RunDir = i_run_dir;
            //         state.GlobalConfig = cgui_state.GlobalConfig;
            //         state.GroupConfigs = cgui_state.GroupConfigs;
            //         state.CurrentState =
            //             SiPMAcquisitionStates::AttemptConnection;
            //         return true;
            //     }
            // )) {
            //     c_connected_mod = 0.5;
            // }

            ImGui::PopStyleColor(3);

            /// Disconnect button
            float c_disconnected_mod = 1.5f - c_connected_mod;
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button,
                static_cast<ImVec4>(ImColor::HSV(0.0f, 0.6f, c_disconnected_mod*0.6f)));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                static_cast<ImVec4>(ImColor::HSV(0.0f, 0.7f, c_disconnected_mod*0.7f)));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                static_cast<ImVec4>(ImColor::HSV(0.0f, 0.8f, c_disconnected_mod*0.8f)));

            // if (CAENControlFac.Button("Disconnect##caen",
            //     [=](SiPMAcquisitionData& state) {
            //         // Only change the state if any of these states
            //         if(state.CurrentState == SiPMAcquisitionStates::OscilloscopeMode ||
            //             state.CurrentState == SiPMAcquisitionStates::MeasurementRoutineMode) {
            //                 spdlog::info("Going to disconnect the CAEN digitizer");
            //                 state.CurrentState = SiPMAcquisitionStates::Disconnected;
            //         }
            //         return true;
            //     }
            // )) {
            //     // Local stuff
            //     c_connected_mod = 1.5f;
            // }
            draw_at_spot(_sipm_doe, SiPMControls, "Disconnect##CAEN",
                tmp, Button, [&](){ return tmp; },
                // Callback when tmp is true !
                [doe = _sipm_doe]
                (SiPMAcquisitionData& caen_twin) {
                    if(caen_twin.CurrentState == SiPMAcquisitionStates::OscilloscopeMode ||
                        caen_twin.CurrentState == SiPMAcquisitionStates::MeasurementRoutineMode) {
                            caen_twin.CurrentState = SiPMAcquisitionStates::Disconnected;
                    }
            });

            ImGui::PopStyleColor(3);

            ImGui::Separator();

            static float o_connected_mod = 1.5f;
            static std::string other_port = _slowdaq_doe.PFEIFFERPort;
            ImGui::InputText("PFEIFFER port", &other_port);
            draw_at_spot(_slowdaq_doe, SlowDAQControls, "PFEIFFER Port",
                _slowdaq_doe.PFEIFFERPort, InputText, ImGui::IsItemEdited,
                // Callback when tmp is true !
                [doe = _slowdaq_doe]
                (SlowDAQData& doe_twin) {
                    doe_twin.PFEIFFERPort = doe.PFEIFFERPort;
            });

            ImGui::SameLine(300);
            // Colors to pop up or shadow it depending on the conditions
            ImGui::PushStyleColor(ImGuiCol_Button,
                static_cast<ImVec4>(ImColor::HSV(2.0f / 7.0f, 0.6f, o_connected_mod*0.6f)));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                static_cast<ImVec4>(ImColor::HSV(2.0f / 7.0f, 0.7f, o_connected_mod*0.7f)));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                static_cast<ImVec4>(ImColor::HSV(2.0f / 7.0f, 0.8f, o_connected_mod*0.8f)));

            // This button starts the CAEN communication and sends all
            // the setup configuration
            // if (SlowDAQControlFac.Button("Connect##sdaq",
            //     [=](SlowDAQData& state) {
            //         state = _slowdaq_doe;
            //         state.RunDir = i_run_dir;

            //         // Pfeiffer
            //         state.PFEIFFERPort = other_port;
            //         state.PFEIFFERState =
            //             PFEIFFERSSGState::AttemptConnection;

            //         return true;
            //     }
            // )) {
            //     spdlog::info("Hello");
            //     o_connected_mod = 0.5f;
            // }

            draw_at_spot(_slowdaq_doe, SlowDAQControls, "Connect##SLOWDAQ",
                tmp, Button, [&](){ return tmp; },
                // Callback when tmp is true !
                [doe = _slowdaq_doe]
                (SlowDAQData& doe_twin) {
                    doe_twin.PFEIFFERState = PFEIFFERSSGState::AttemptConnection;

            });

            ImGui::PopStyleColor(3);

                            /// Disconnect button
            float o_disconnected_mod = 1.5f - o_connected_mod;
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button,
                static_cast<ImVec4>(ImColor::HSV(0.0f, 0.6f, o_disconnected_mod*0.6f)));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                static_cast<ImVec4>(ImColor::HSV(0.0f, 0.7f, o_disconnected_mod*0.7f)));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                static_cast<ImVec4>(ImColor::HSV(0.0f, 0.8f, o_disconnected_mod*0.8f)));

            // if (SlowDAQControlFac.Button("Disconnect##sdaq",
            //     [=](SlowDAQData& state) {
            //         // Only change the state if any of these states
            //         if (state.PFEIFFERState == PFEIFFERSSGState::AttemptConnection ||
            //             state.PFEIFFERState == PFEIFFERSSGState::Connected){
            //                 state.PFEIFFERState = PFEIFFERSSGState::Disconnected;
            //         }
            //         return true;
            //     }
            // )) {
            //     // Local stuff
            //     o_connected_mod = 1.5f;
            // }
            draw_at_spot(_slowdaq_doe, SlowDAQControls, "Disconnect##SLOWDAQ",
                tmp, Button, [&](){ return tmp; },
                // Callback when tmp is true !
                [doe = _slowdaq_doe]
                (SlowDAQData& doe_twin) {
                    if (doe_twin.PFEIFFERState == PFEIFFERSSGState::AttemptConnection ||
                        doe_twin.PFEIFFERState == PFEIFFERSSGState::Connected){
                            doe_twin.PFEIFFERState = PFEIFFERSSGState::Disconnected;
                    }
            });

            ImGui::PopStyleColor(3);

            ImGui::PopItemWidth();

            ImGui::Separator();


                // _teensyQueueF([=](TeensyControllerState& oldState) {
                //  // There is nothing to do here technically
                //  // but I am including it just in case
                //  return true;
                // });
                // _caenQueueF();


            ImGui::EndTabItem();
        }

        _gui_controls_tab();
        ttabs();
        ctabs();

        ImGui::EndTabBar();
    }
    ImGui::End();
}

template<class Pipes>
void ControlWindow<Pipes>::_gui_controls_tab() {
    if (ImGui::BeginTabItem("GUI")) {
        ImGui::SliderFloat("Plot line-width",
            &ImPlot::GetStyle().LineWeight, 0.0f, 10.0f);
        ImGui::SliderFloat("Plot Default Size Y",
            &ImPlot::GetStyle().PlotDefaultSize.y, 0.0f, 1000.0f);

        ImGui::EndTabItem();
    }
}

template<typename Pipes>
ControlWindow<Pipes> make_control_window(const Pipes& p) {
    return ControlWindow<Pipes>(p);
}

}  // namespace SBCQueens

#endif
