#include "GUI/ControlWindow.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

namespace SBCQueens {

	void ControlWindow::init(const toml::table& tb) {
		_config_table = tb;
		auto t_conf = _config_table["Teensy"];
		auto CAEN_conf = _config_table["CAEN"];
		auto other_conf = _config_table["Other"];
		auto file_conf = _config_table["File"];

		// These two guys are shared between CAEN and Teensy
		i_run_dir 	= _config_table["File"]["RunDir"].value_or("./RUNS");
		i_run_name 	= _config_table["File"]["RunName"].value_or("Testing");

		/// Teensy config
		// Teensy initial state
		tgui_state.CurrentState = TeensyControllerStates::Standby;
		tgui_state.Port 		= t_conf["Port"].value_or("COM4");
		tgui_state.RunDir 		= i_run_dir;
		tgui_state.RunName 		= i_run_name;

		/// Teensy RTD config
		tgui_state.RTDOnlyMode
			= t_conf["RTDMode"].value_or(false);
		tgui_state.RTDSamplingPeriod
			= t_conf["RTDSamplingPeriod"].value_or(100Lu);
		tgui_state.RTDMask
			= t_conf["RTDMask"].value_or(0xFFFFLu);

		/// Teensy PID config
		// Send initial states of the PIDs
		tgui_state.PeltierState = false;

		tgui_state.PIDTempTripPoint
			= t_conf["PeltierTempTripPoint"].value_or(0.0f);
		tgui_state.PIDTempValues.SetPoint
			= t_conf["PeltierTempSetpoint"].value_or(0.0f);
		tgui_state.PIDTempValues.Kp
			= t_conf["PeltierTKp"].value_or(0.0f);
		tgui_state.PIDTempValues.Ti
			= t_conf["PeltierTTi"].value_or(0.0f);
		tgui_state.PIDTempValues.Td
			= t_conf["PeltierTTd"].value_or(0.0f);

		/// CAEN config
		// CAEN initial state
		cgui_state.CurrentState = CAENInterfaceStates::Standby;
		cgui_state.RunDir = i_run_dir;
		cgui_state.RunName = i_run_name;
		cgui_state.SiPMParameters = file_conf["SiPMParameters"].value_or("default");

		/// CAEN model configs
		cgui_state.Model
			= CAENDigitizerModels_map.at(CAEN_conf["Model"].value_or("DT5730B"));
		cgui_state.PortNum
			= CAEN_conf["Port"].value_or(0u);
		cgui_state.GlobalConfig.MaxEventsPerRead
			= CAEN_conf["MaxEventsPerRead"].value_or(512Lu);
		cgui_state.GlobalConfig.RecordLength
			= CAEN_conf["RecordLength"].value_or(2048Lu);
		cgui_state.GlobalConfig.PostTriggerPorcentage
			= CAEN_conf["PostBufferPorcentage"].value_or(50u);
		cgui_state.GlobalConfig.TriggerOverlappingEn
			= CAEN_conf["OverlappingRejection"].value_or(false);
		cgui_state.GlobalConfig.EXTasGate
			= CAEN_conf["TRGINasGate"].value_or(false);
		cgui_state.GlobalConfig.EXTTriggerMode
			= static_cast<CAEN_DGTZ_TriggerMode_t>(CAEN_conf["ExternalTrigger"].value_or(0L));
		cgui_state.GlobalConfig.SWTriggerMode
			= static_cast<CAEN_DGTZ_TriggerMode_t>(CAEN_conf["SoftwareTrigger"].value_or(0L));
		cgui_state.GlobalConfig.TriggerPolarity
			= static_cast<CAEN_DGTZ_TriggerPolarity_t>(CAEN_conf["Polarity"].value_or(0L));
		cgui_state.GlobalConfig.IOLevel
			= static_cast<CAEN_DGTZ_IOLevel_t>(CAEN_conf["IOLevel"].value_or(0));
		// We check how many CAEN.groupX there are and create that many
		// groups.
		const uint8_t c_MAX_CHANNELS = 64;
		for(uint8_t ch = 0; ch < c_MAX_CHANNELS; ch++) {
			std::string ch_toml = "group" + std::to_string(ch);
			if(CAEN_conf[ch_toml].as_table()) {

				cgui_state.GroupConfigs.emplace_back(
					CAENGroupConfig{
						.Number = ch,
						.TriggerMask = CAEN_conf[ch_toml]["TrgMask"].value_or<uint8_t>(0),
						.AcquisitionMask = CAEN_conf[ch_toml]["AcqMask"].value_or<uint8_t>(0),
						.DCOffset = CAEN_conf[ch_toml]["Offset"].value_or<uint16_t>(0x8000u),
						.DCRange = CAEN_conf[ch_toml]["Range"].value_or<uint8_t>(0u),
						.TriggerThreshold = CAEN_conf[ch_toml]["Threshold"].value_or<uint16_t>(0x8000u)
					}
				);

				if(toml::array* arr = CAEN_conf[ch_toml]["Corrections"].as_array()) {
					spdlog::info("Corrections exist");
					size_t j = 0;
					CAENGroupConfig& last_config = cgui_state.GroupConfigs.back();
					for(toml::node& elem : *arr) {
						// Max number of channels per group there can be is 8
						if(j < 8){
							last_config.DCCorrections.emplace_back(
								elem.value_or(0u)
							);
							j++;
						}
					}
				}

			}
		}

		/// Other config
		other_state.RunDir = i_run_dir;
		other_state.RunName = i_run_name;

		/// Other PFEIFFERSingleGauge
		other_state.PFEIFFERPort = other_conf["PFEIFFERSingleGauge"]["Port"].value_or("");
		other_state.PFEIFFERSingleGaugeEnable
			= other_conf["PFEIFFERSingleGauge"]["Enabled"].value_or(false);
		other_state.PFEIFFERSingleGaugeUpdateSpeed
			= static_cast<PFEIFFERSingleGaugeSP>(
				other_conf["PFEIFFERSingleGauge"]["Enabled"].value_or(0)
			);

	}

	bool ControlWindow::operator()() {

		ImGui::Begin("Control Window");

		if (ImGui::BeginTabBar("ControlTabs")) {

			if(ImGui::BeginTabItem("Run")) {

				ImGui::InputText("Runs Directory", &i_run_dir);
				if(ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Directory of where all the run "
						"files are going to be saved at.\n"
						"Fixed when the connect buttons are pressed.");
				}

				ImGui::InputText("Run name", &i_run_name);
				if(ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Name of the run.\n"
						"Fixed when the connect buttons are pressed.");
				}

				ImGui::InputText("SiPM run name", &cgui_state.SiPMParameters);
				if(ImGui::IsItemHovered()) {
					ImGui::SetTooltip("This will appended to the name "
						"of the SiPM pulse file to denote that the these "
						"SiPM pulses where taken during this run but "
						"with different parameters (OV for example).\n"
						"Fixed when the Start SiPM data taking buttons "
						"is pressed.");
				}


				ImGui::Separator();
				ImGui::PushItemWidth(100);
				// 1.0 -> 0.5
				// 0.5 -> 1.0
				// (1.0 - 0.5) / (0.5 - 1.0) = -1
				static float connected_mod = 1.5;
				// Com port text input
				// We do not make this a GUI queue item because
				// we only need to let the client know when we click connnect
				ImGui::InputText("Teensy COM port", &tgui_state.Port);

				ImGui::SameLine(300);
				// Colors to pop up or shadow it depending on the conditions
				ImGui::PushStyleColor(ImGuiCol_Button,
					static_cast<ImVec4>(ImColor::HSV(2.0 / 7.0f, 0.6f, connected_mod*0.6f)));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
					static_cast<ImVec4>(ImColor::HSV(2.0 / 7.0f, 0.7f, connected_mod*0.7f)));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,
					static_cast<ImVec4>(ImColor::HSV(2.0 / 7.0f, 0.8f, connected_mod*0.8f)));
				// The operator () carries the ImGUI drawing functions
				// and the task it sends to its associated queue/thread
				// is the lambda (callback) we pass
				//
				// The double ## and the name is necessary if several
				// controls share a same name in the same context/scope
				// otherwise, ImGUI will not differentiate between them
				// and causes issues.
				if(TeensyControlFac.Button("Connect##teensy",
					// To make things secure, we pass everything by value
					[=](TeensyControllerData& oldState) {
						// We copy the local state.
						oldState = tgui_state;
						oldState.RunDir = i_run_dir;
						oldState.RunName = i_run_name;
						// Except for the fact we are changing the state
						oldState.CurrentState
							= TeensyControllerStates::AttemptConnection;
						return true;
					}
				// This is code that is run locally to this thread
				// when the button is pressed
				)) {
					spdlog::info("Hello");
					connected_mod = 0.5;
				}

				ImGui::PopStyleColor(3);

				/// Disconnect button
				float disconnected_mod = 1.5 - connected_mod;
				ImGui::SameLine();
				ImGui::PushStyleColor(ImGuiCol_Button,
					static_cast<ImVec4>(ImColor::HSV(0.0f, 0.6f, disconnected_mod*0.6f)));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
					static_cast<ImVec4>(ImColor::HSV(0.0f, 0.7f, disconnected_mod*0.7f)));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,
					static_cast<ImVec4>(ImColor::HSV(0.0, 0.8f, disconnected_mod*0.8f)));

				if(TeensyControlFac.Button("Disconnect##teensy",
					[=](TeensyControllerData& oldState) {
						// No need to disconnect if its not connected
						if(oldState.CurrentState == TeensyControllerStates::Connected) {
							oldState.CurrentState
								= TeensyControllerStates::Disconnected;
						}
						return true;
					}
				)) {
					connected_mod = 1.5;
				}

				ImGui::PopStyleColor(3);
				ImGui::Separator();

				static float c_connected_mod = 1.5;
				static int caen_port = cgui_state.PortNum;
				ImGui::InputInt("CAEN port", &cgui_state.PortNum);
		        if (ImGui::IsItemHovered()) {
        			ImGui::SetTooltip("Usually 0 as long as there is no "
        			"other CAEN digitizers connected. If there are more, "
        			"the port increases as they were connected to the "
        			"computer.");
		        } ImGui::SameLine();

    			CAENControlFac.ComboBox("Connection Type",
					cgui_state.ConnectionType,
					{{CAENConnectionType::USB, "USB"},
					{CAENConnectionType::A4818, "A4818 USB 3.0 to CONET Adapter"}},
					[]() {return false;}, [](){});

				if(cgui_state.ConnectionType == CAENConnectionType::A4818) {
					ImGui::SameLine();
					ImGui::InputScalar("VME Address", ImGuiDataType_U32, &cgui_state.VMEAddress);
				}

				// ImGui::SameLine(300);
				// Colors to pop up or shadow it depending on the conditions
				ImGui::PushStyleColor(ImGuiCol_Button,
					static_cast<ImVec4>(ImColor::HSV(2.0 / 7.0f, 0.6f, c_connected_mod*0.6f)));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
					static_cast<ImVec4>(ImColor::HSV(2.0 / 7.0f, 0.7f, c_connected_mod*0.7f)));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,
					static_cast<ImVec4>(ImColor::HSV(2.0 / 7.0f, 0.8f, c_connected_mod*0.8f)));

				// This button starts the CAEN communication and sends all
				// the setup configuration
				if(CAENControlFac.Button("Connect##caen",
					[=](CAENInterfaceData& state) {

						state = cgui_state;
						state.RunDir = i_run_dir;
						state.RunName = i_run_name;
						state.GlobalConfig = cgui_state.GlobalConfig;
						state.GroupConfigs = cgui_state.GroupConfigs;
						state.CurrentState =
							CAENInterfaceStates::AttemptConnection;
						return true;

					}
				)) {
					spdlog::info("Hello");
					c_connected_mod = 0.5;
				}

				ImGui::PopStyleColor(3);

								/// Disconnect button
				float c_disconnected_mod = 1.5 - c_connected_mod;
				ImGui::SameLine();
				ImGui::PushStyleColor(ImGuiCol_Button,
					static_cast<ImVec4>(ImColor::HSV(0.0f, 0.6f, c_disconnected_mod*0.6f)));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
					static_cast<ImVec4>(ImColor::HSV(0.0f, 0.7f, c_disconnected_mod*0.7f)));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,
					static_cast<ImVec4>(ImColor::HSV(0.0, 0.8f, c_disconnected_mod*0.8f)));

				if(CAENControlFac.Button("Disconnect##caen",
					[=](CAENInterfaceData& state) {
						// Only change the state if any of these states
						if(state.CurrentState == CAENInterfaceStates::OscilloscopeMode ||
							state.CurrentState == CAENInterfaceStates::StatisticsMode ||
							state.CurrentState == CAENInterfaceStates::RunMode){
								spdlog::info("Going to disconnect the CAEN digitizer");
								state.CurrentState = CAENInterfaceStates::Disconnected;
						}
						return true;

					}
				)) {
					// Local stuff
					c_connected_mod = 1.5;
				}

				ImGui::PopStyleColor(3);
				ImGui::Separator();


				static float o_connected_mod = 1.5;
				static std::string other_port = other_state.PFEIFFERPort;
				ImGui::InputText("PFEIFFER port", &other_port);

				ImGui::SameLine(300);
				// Colors to pop up or shadow it depending on the conditions
				ImGui::PushStyleColor(ImGuiCol_Button,
					static_cast<ImVec4>(ImColor::HSV(2.0 / 7.0f, 0.6f, o_connected_mod*0.6f)));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
					static_cast<ImVec4>(ImColor::HSV(2.0 / 7.0f, 0.7f, o_connected_mod*0.7f)));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,
					static_cast<ImVec4>(ImColor::HSV(2.0 / 7.0f, 0.8f, o_connected_mod*0.8f)));

				// This button starts the CAEN communication and sends all
				// the setup configuration
				if(SlowDAQControlFac.Button("Connect##sdaq",
					[=](OtherDevicesData& state) {

						state = other_state;
						state.RunDir = i_run_dir;
						state.RunName = i_run_name;

						// Pfeiffer
						state.PFEIFFERPort = other_port;
						state.PFEIFFERState =
							PFEIFFERSSGState::AttemptConnection;

						return true;

					}
				)) {
					spdlog::info("Hello");
					o_connected_mod = 0.5;
				}

				ImGui::PopStyleColor(3);

								/// Disconnect button
				float o_disconnected_mod = 1.5 - o_connected_mod;
				ImGui::SameLine();
				ImGui::PushStyleColor(ImGuiCol_Button,
					static_cast<ImVec4>(ImColor::HSV(0.0f, 0.6f, o_disconnected_mod*0.6f)));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
					static_cast<ImVec4>(ImColor::HSV(0.0f, 0.7f, o_disconnected_mod*0.7f)));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,
					static_cast<ImVec4>(ImColor::HSV(0.0, 0.8f, o_disconnected_mod*0.8f)));

				if(SlowDAQControlFac.Button("Disconnect##sdaq",
					[=](OtherDevicesData& state) {
						// Only change the state if any of these states
						if(state.PFEIFFERState == PFEIFFERSSGState::AttemptConnection ||
							state.PFEIFFERState == PFEIFFERSSGState::Connected){
								spdlog::info("Going to disconnect the PFEIFFER");
								state.PFEIFFERState = PFEIFFERSSGState::Disconnected;
						}
						return true;

					}
				)) {
					// Local stuff
					o_connected_mod = 1.5;
				}
				ImGui::PopStyleColor(3);

				ImGui::PopItemWidth();

				ImGui::Separator();

				// This is the only button that does send a task to all
				// (so far) threads.
				// TODO(Hector): generalize this in the future
				CAENControlFac.Button(
					"Start SiPM Data Taking",
					[=](CAENInterfaceData& state) {
					// Only change state if its in a work related
					// state, i.e oscilloscope mode
					if(state.CurrentState == CAENInterfaceStates::OscilloscopeMode ||
						state.CurrentState == CAENInterfaceStates::StatisticsMode) {
						state.CurrentState = CAENInterfaceStates::RunMode;
						state.SiPMParameters = cgui_state.SiPMParameters;
					}
					return true;
				});

				CAENControlFac.Button(
					"Stop SiPM Data Taking",
					[=](CAENInterfaceData& state) {
					if(state.CurrentState == CAENInterfaceStates::RunMode) {
						state.CurrentState = CAENInterfaceStates::OscilloscopeMode;
						state.SiPMParameters = cgui_state.SiPMParameters;
					}
					return true;
				});
					// _teensyQueueF([=](TeensyControllerState& oldState) {
					// 	// There is nothing to do here technically
					// 	// but I am including it just in case
					// 	return true;
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

		return true;
	}

	void ControlWindow::_gui_controls_tab() {
		if(ImGui::BeginTabItem("GUI")) {

			ImGui::SliderFloat("Plot line-width", &ImPlot::GetStyle().LineWeight, 0.0f, 10.0f);
			ImGui::SliderFloat("Plot Default Size Y", &ImPlot::GetStyle().PlotDefaultSize.y, 0.0f, 1000.0f);

			ImGui::EndTabItem();
		}
	}


} // namespace SBCQueens