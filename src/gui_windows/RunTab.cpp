#include "sbcqueens-gui/gui_windows/RunTab.hpp"
#include "sbcqueens-gui/imgui_helpers.hpp"

namespace SBCQueens {

void RunTab::init_tab(const toml::table& cfg) {
    auto t_conf = cfg["Teensy"];
    auto CAEN_conf = cfg["CAEN"];
    auto other_conf = cfg["Other"];
    auto file_conf = cfg["File"];
		i_run_dir  = file_conf["RunDir"].value_or("./RUNS");

		_teensy_doe.RunDir = i_run_dir;
		_sipm_doe.RunDir = i_run_dir;
		_slowdaq_doe.RunDir = i_run_dir;

		// Teens stuff
    _teensy_doe.Port = t_conf["Port"].value_or("COM3");

    // CAEN Run stuff
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

    // Other/slow daq stuff
    _slowdaq_doe.PFEIFFERPort
  		= other_conf["PFEIFFERSingleGauge"]["Port"].value_or("COM5");
}

void RunTab::draw() {
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
    // ImGui::PushStyleColor(ImGuiCol_Button,
    //     static_cast<ImVec4>(ImColor::HSV(2.0f / 7.0f, 0.6f, connected_mod*0.6f)));
    // ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
    //     static_cast<ImVec4>(ImColor::HSV(2.0f / 7.0f, 0.7f, connected_mod*0.7f)));
    // ImGui::PushStyleColor(ImGuiCol_ButtonActive,
    //     static_cast<ImVec4>(ImColor::HSV(2.0f / 7.0f, 0.8f, connected_mod*0.8f)));
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
    constexpr auto connect_teensy = get_control<ControlTypes::Button,
                                    "Connect##Teensy">(SiPMGUIControls);
    draw_control(connect_teensy, _teensy_doe,
        tmp, [&](){ return tmp; },
        // Callback when IsItemEdited !
        [doe = _teensy_doe, run_dir = i_run_dir]
        (TeensyControllerData& teensy_twin) {
            teensy_twin.RunDir = run_dir;
            teensy_twin.Port = doe.Port;
            teensy_twin.CurrentState
                = TeensyControllerStates::AttemptConnection;
    });

    // ImGui::PopStyleColor(3);

    /// Disconnect button
    float disconnected_mod = 1.5f - connected_mod;
    ImGui::SameLine();
    // ImGui::PushStyleColor(ImGuiCol_Button,
    //     static_cast<ImVec4>(ImColor::HSV(0.0f, 0.6f, disconnected_mod*0.6f)));
    // ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
    //     static_cast<ImVec4>(ImColor::HSV(0.0f, 0.7f, disconnected_mod*0.7f)));
    // ImGui::PushStyleColor(ImGuiCol_ButtonActive,
    //     static_cast<ImVec4>(ImColor::HSV(0.0f, 0.8f, disconnected_mod*0.8f)));

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
    constexpr Control disconnect_teensy = get_control<ControlTypes::Button,
                                    "Disconnect##Teensy">(SiPMGUIControls);
    draw_control(disconnect_teensy, _teensy_doe,
        tmp, [&](){ return tmp; },
        // Callback when IsItemEdited !
        [](TeensyControllerData& teensy_twin) {
            if (teensy_twin.CurrentState == TeensyControllerStates::Connected) {
                teensy_twin.CurrentState
                    = TeensyControllerStates::Disconnected;
            }
    });


    // ImGui::PopStyleColor(3);
    ImGui::Separator();

    static float c_connected_mod = 1.5f;
    //ImGui::InputInt("CAEN port", , cgui_state.PortNum);
    constexpr Control sipm_port = get_control<ControlTypes::InputInt,
                                        "CAEN Port">(SiPMGUIControls);
    draw_control(sipm_port, _sipm_doe,
        _sipm_doe.PortNum, ImGui::IsItemEdited,
        // Callback when IsItemEdited !
        [&](SiPMAcquisitionData& caen_twin) {
          caen_twin.PortNum = _sipm_doe.PortNum;
    });

    ImGui::SameLine();

    // CAENControlFac.ComboBox("Connection Type",
    //     cgui_state.ConnectionType,
    //     {{CAENConnectionType::USB, "USB"},
    //     {CAENConnectionType::A4818, "A4818 USB 3.0 to CONET Adapter"}},
    //     []() {return false;}, [](){});

    // constexpr Control connection_type = get_control(SiPMGUIControls, "Connection Type");
    // draw_control(_sipm_doe, connection_type, ComboBox,
    //     _sipm_doe.ConnectionType, ImGui::IsItemEdited,
    //     // Callback when IsItemEdited !
    //     [doe = _sipm_doe](SiPMAcquisitionData& caen_twin) {
    //       caen_twin.ConnectionType = doe.ConnectionType;
    //     },
    //     {{CAENConnectionType::USB, "USB"},
    //     {CAENConnectionType::A4818, "A4818 USB 3.0 to CONET Adapter"}});

    if (_sipm_doe.ConnectionType == CAENConnectionType::A4818) {
        ImGui::SameLine();
        // ImGui::InputScalar("VME Address", ImGuiDataType_U32,
        //     &cgui_state.VMEAddress);
        constexpr auto caen_model = get_control<ControlTypes::InputUINT32,
            "VME Address">(SiPMGUIControls);
        draw_control(caen_model, _sipm_doe,
            _sipm_doe.VMEAddress, ImGui::IsItemEdited,
            // Callback when IsItemEdited !
            [doe = _sipm_doe](SiPMAcquisitionData& caen_twin) {
              caen_twin.VMEAddress = doe.VMEAddress;
        });

    }

    // ImGui::InputText("Keithley COM Port",
    //     &cgui_state.SiPMVoltageSysPort);
    constexpr auto keith_com = get_control<ControlTypes::InputText,
                                    "Keithley COM Port">(SiPMGUIControls);
    draw_control(keith_com, _sipm_doe,
        _sipm_doe.SiPMVoltageSysPort, ImGui::IsItemEdited,
        // Callback when IsItemEdited !
        [&](SiPMAcquisitionData& caen_twin) {
          caen_twin.SiPMVoltageSysPort = _sipm_doe.SiPMVoltageSysPort;
    });

    // ImGui::SameLine(300);
    // Colors to pop up or shadow it depending on the conditions

    // This button starts the CAEN communication and sends all
    // the setup configuration
    constexpr auto connect_caen = get_control<ControlTypes::Button,
                                        "Connect##CAEN">(SiPMGUIControls);
    draw_control(connect_caen, _sipm_doe,
        tmp, [&](){ return tmp; },
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

    constexpr auto disconnect_caen = get_control<ControlTypes::Button,
                                        "Disconnect##CAEN">(SiPMGUIControls);
    draw_control(disconnect_caen, _sipm_doe,
        tmp, [&](){ return tmp; },
        // Callback when tmp is true !
        [](SiPMAcquisitionData& caen_twin) {
            if(caen_twin.CurrentState == SiPMAcquisitionStates::OscilloscopeMode ||
                caen_twin.CurrentState == SiPMAcquisitionStates::MeasurementRoutineMode) {
                    caen_twin.CurrentState = SiPMAcquisitionStates::Disconnected;
            }
    });

    ImGui::PopStyleColor(3);

    ImGui::Separator();

    static float o_connected_mod = 1.5f;
    static std::string other_port = _slowdaq_doe.PFEIFFERPort;
    // ImGui::InputText("PFEIFFER port", &other_port);

    constexpr auto pfeiffer_port = get_control<ControlTypes::InputText,
                                        "PFEIFFER Port">(SiPMGUIControls);
    draw_control(pfeiffer_port, _slowdaq_doe,
        _slowdaq_doe.PFEIFFERPort, ImGui::IsItemEdited,
        // Callback when tmp is true !
        [&](SlowDAQData& doe_twin) {
            doe_twin.PFEIFFERPort = _slowdaq_doe.PFEIFFERPort;
    });

    ImGui::SameLine(300);
    // Colors to pop up or shadow it depending on the conditions
    // ImGui::PushStyleColor(ImGuiCol_Button,
    //     static_cast<ImVec4>(ImColor::HSV(2.0f / 7.0f, 0.6f, o_connected_mod*0.6f)));
    // ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
    //     static_cast<ImVec4>(ImColor::HSV(2.0f / 7.0f, 0.7f, o_connected_mod*0.7f)));
    // ImGui::PushStyleColor(ImGuiCol_ButtonActive,
    //     static_cast<ImVec4>(ImColor::HSV(2.0f / 7.0f, 0.8f, o_connected_mod*0.8f)));

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
    constexpr auto connect_slowdaq = get_control<ControlTypes::Button,
                                    "Connect##SLOWDAQ">(SiPMGUIControls);
    draw_control(connect_slowdaq, _slowdaq_doe,
        tmp, [&](){ return tmp; },
        // Callback when tmp is true !
        [](SlowDAQData& doe_twin) {
            doe_twin.PFEIFFERState = PFEIFFERSSGState::AttemptConnection;
    });

    // ImGui::PopStyleColor(3);

                    /// Disconnect button
    float o_disconnected_mod = 1.5f - o_connected_mod;
    ImGui::SameLine();
    // ImGui::PushStyleColor(ImGuiCol_Button,
    //     static_cast<ImVec4>(ImColor::HSV(0.0f, 0.6f, o_disconnected_mod*0.6f)));
    // ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
    //     static_cast<ImVec4>(ImColor::HSV(0.0f, 0.7f, o_disconnected_mod*0.7f)));
    // ImGui::PushStyleColor(ImGuiCol_ButtonActive,
    //     static_cast<ImVec4>(ImColor::HSV(0.0f, 0.8f, o_disconnected_mod*0.8f)));

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
    constexpr auto disconnect_slowdaq = get_control<ControlTypes::Button,
                                    "Disconnect##SLOWDAQ">(SiPMGUIControls);
    draw_control(disconnect_slowdaq, _slowdaq_doe,
        tmp, [&](){ return tmp; },
        // Callback when tmp is true !
        [](SlowDAQData& doe_twin) {
            if (doe_twin.PFEIFFERState == PFEIFFERSSGState::AttemptConnection ||
                doe_twin.PFEIFFERState == PFEIFFERSSGState::Connected){
                    doe_twin.PFEIFFERState = PFEIFFERSSGState::Disconnected;
            }
    });

    // ImGui::PopStyleColor(3);

    ImGui::PopItemWidth();

    ImGui::Separator();
}

} // namespace SBCQueens