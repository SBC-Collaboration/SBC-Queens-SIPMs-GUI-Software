#pragma once

#include <math.h>
#include <utility>
#include <vector>
#include <iostream>
#include <tuple>
#include <functional>

#include <imgui.h>
#include <implot.h>
#include <spdlog/spdlog.h>
#include <toml.hpp>

#include "TeensyControllerInterface.h"
#include "CAENDigitizerInterface.h"
#include "deps/imgui/imgui.h"
#include "imgui.h"
#include "imgui_helpers.h"
#include "implot_helpers.h"
#include "include/caen_helper.h"
#include "include/imgui_helpers.h"
#include "include/implot_helpers.h"
#include "include/indicators.h"
#include "indicators.h"

namespace SBCQueens {

	template<typename... QueueFuncs>
	class GUIManager
	{
private:

		std::tuple<QueueFuncs&...> _queues;
		IndicatorReceiver<IndicatorNames> _plotManager;

		// Controls
		Button connect_btn;
		Button disconnect_btn;

		Checkbox pid_relay_chkBox;
		Checkbox gen_relay_chkBox;

		// PID1 Controls
		Checkbox pid_one_chkBox;
		InputFloat pid_one_temp_sp_inpF;
		InputFloat pid_one_curr_sp_inpF;
		InputFloat pid_one_PIDTKP_inpF;
		InputFloat pid_one_PIDTTI_inpF;
		InputFloat pid_one_PIDTTd_inpF;

		InputFloat pid_one_PIDAKP_inpF;
		InputFloat pid_one_PIDATI_inpF;
		InputFloat pid_one_PIDATD_inpF;

		// PID2 Controls
		Checkbox pid_two_chkBox;
		InputFloat pid_two_temp_sp_inpF;
		InputFloat pid_two_curr_sp_inpF;
		InputFloat pid_two_PIDTKP_inpF;
		InputFloat pid_two_PIDTTI_inpF;
		InputFloat pid_two_PIDTTd_inpF;

		InputFloat pid_two_PIDAKP_inpF;
		InputFloat pid_two_PIDATI_inpF;
		InputFloat pid_two_PIDATD_inpF;

		// CAEN Controls
		Button caen_connect_btn;
		Button caen_disconnect_btn;
		Button send_soft_trig;
		Button start_processing_btn;

		// Indicators

		// Teensy
			TeensyIndicator latest_pid1_temp;
			TeensyIndicator latest_pid1_curr;
			TeensyIndicator latest_pid2_temp;
			TeensyIndicator latest_pid2_curr;

			TeensyIndicator latest_box_bme_hum;
			TeensyIndicator latest_box_bme_temp;
		// End Teensy

		// CAEN
			SiPMIndicator eventsBuffer;
			SiPMIndicator frequency;
			SiPMIndicator dark_noise;
			SiPMIndicator gain;
		// End CAEN

		// Plots
		SiPMPlot pid1_temps_plot;
		SiPMPlot pid1_currs_plot;
		SiPMPlot pid2_temps_plot;
		SiPMPlot pid2_currs_plot;

		SiPMPlot local_bme_temp_plot;
		SiPMPlot local_bme_pres_plot;
		SiPMPlot local_bme_humi_plot;

		SiPMPlot box_bme_temp_plot;
		SiPMPlot box_bme_pres_plot;
		SiPMPlot box_bme_humi_plot;

		SiPMPlot sipm_plot;

public:
		explicit GUIManager(QueueFuncs&... queues) : 
			_queues(forward_as_tuple(queues...)),
			_plotManager(std::get<SiPMsPlotQueue&>(_queues)) {

			// Controls
			auto config = toml::parse_file("gui_setup.toml");

			auto teensyConf = config["Teensy"];
			connect_btn = make_button("Connect");
			disconnect_btn = make_button("Disconnect");

			pid_relay_chkBox = make_checkbox("PID Relay State",
				teensyConf["PIDRelay"].value_or(false));
			gen_relay_chkBox = make_checkbox("General Relay State",
				teensyConf["GeneralRelay"].value_or(false));

			// PID1
			pid_one_chkBox = make_checkbox("PID1 Enable",
				teensyConf["PID1Enable"].value_or(false));

			pid_one_temp_sp_inpF = make_input_float("Temp Setpoint 1",
				teensyConf["PID1TempSetpoint"].value_or(0.0f));
			pid_one_curr_sp_inpF = make_input_float("Current Setpoint 1",
				teensyConf["PID1CurrentSetpoint"].value_or(3.0f));

			pid_one_PIDTKP_inpF = make_input_float("Tkp 1",
				teensyConf["PID1TKp"].value_or(1000.0f));
			pid_one_PIDTTI_inpF = make_input_float("Tki 1",
				teensyConf["PID1TTi"].value_or(100.0f));
			pid_one_PIDTTd_inpF = make_input_float("Tkd 1",
				teensyConf["PID1TTd"].value_or(0.0f));

			pid_one_PIDAKP_inpF = make_input_float("Akp 1",
				teensyConf["PID1AKp"].value_or(200.0f));
			pid_one_PIDATI_inpF = make_input_float("Aki 1",
				teensyConf["PID1ATi"].value_or(100.0f));
			pid_one_PIDATD_inpF = make_input_float("Akd 1",
				teensyConf["PID1ATd"].value_or(0.0f));

			pid_two_chkBox = make_checkbox("PID2 Enable",
				teensyConf["PID2Enable"].value_or(false));
			pid_two_temp_sp_inpF = make_input_float("Temp Setpoint 2",
				teensyConf["PID2TempSetpoint"].value_or(0.0f));
			pid_two_curr_sp_inpF = make_input_float("Current Setpoint 2",
				teensyConf["PID2CurrentSetpoint"].value_or(3.0f));

			pid_two_PIDTKP_inpF = make_input_float("Tkp 2",
				teensyConf["PID2TKp"].value_or(1000.0f));
			pid_two_PIDTTI_inpF = make_input_float("Tki 2",
				teensyConf["PID2TTi"].value_or(100.0f));
			pid_two_PIDTTd_inpF = make_input_float("Tkd 2",
				teensyConf["PID2TTd"].value_or(0.0f));

			pid_two_PIDAKP_inpF = make_input_float("Akp 2",
				teensyConf["PID2AKp"].value_or(200.0f));
			pid_two_PIDATI_inpF = make_input_float("Aki 2",
				teensyConf["PID2ATi"].value_or(100.0f));
			pid_two_PIDATD_inpF = make_input_float("Akd 2",
				teensyConf["PID2ATd"].value_or(0.0f));
			// End Teensy Controls

			// CAEN Controls
			caen_connect_btn = make_button("Connect");
			caen_disconnect_btn = make_button("Disconnect");
			send_soft_trig = make_button("Software Trigger");
			start_processing_btn = make_button("Start Processing");

			// Indicators

			// Teensy
			latest_pid1_temp = make_indicator(IndicatorNames::LATEST_PID1_TEMP);
			_plotManager.add(latest_pid1_temp);

			latest_pid1_curr = make_indicator(IndicatorNames::LATEST_PID1_CURR);
			_plotManager.add(latest_pid1_curr);
			latest_pid2_temp = make_indicator(IndicatorNames::LATEST_PID2_TEMP);
			_plotManager.add(latest_pid2_temp);
			latest_pid2_curr = make_indicator(IndicatorNames::LATEST_PID2_CURR);
			_plotManager.add(latest_pid2_curr);

			latest_box_bme_hum = make_indicator(IndicatorNames::LATEST_BOX_BME_HUM);
			_plotManager.add(latest_box_bme_hum);
			latest_box_bme_temp = make_indicator(IndicatorNames::LATEST_BOX_BME_TEMP);
			_plotManager.add(latest_box_bme_temp);
			// End Teensy

			// CAEN
			eventsBuffer = make_indicator(IndicatorNames::CAENBUFFEREVENTS, 4,
				NumericFormat::Default);
			_plotManager.add(eventsBuffer);
			frequency = make_indicator(IndicatorNames::FREQUENCY, 4, NumericFormat::Scientific);
			_plotManager.add(frequency);
			dark_noise = make_indicator(IndicatorNames::DARK_NOISE_RATE, 3, NumericFormat::Scientific);
			_plotManager.add(dark_noise);
			gain = make_indicator(IndicatorNames::GAIN, 3, NumericFormat::Scientific);
			_plotManager.add(gain);
			// End CAEN

			// Plots
			pid1_temps_plot = make_plot(IndicatorNames::PID1_Temps);
			_plotManager.add(pid1_temps_plot);
			pid1_currs_plot = make_plot(IndicatorNames::PID1_Currs);
			_plotManager.add(pid1_currs_plot);
			pid2_temps_plot = make_plot(IndicatorNames::PID2_Temps);
			_plotManager.add(pid2_temps_plot);
			pid2_currs_plot = make_plot(IndicatorNames::PID2_Currs);
			_plotManager.add(pid2_currs_plot);

			local_bme_temp_plot = make_plot(IndicatorNames::LOCAL_BME_Temps);
			_plotManager.add(local_bme_temp_plot);
			local_bme_pres_plot = make_plot(IndicatorNames::LOCAL_BME_Pressure);
			_plotManager.add(local_bme_pres_plot);
			local_bme_humi_plot = make_plot(IndicatorNames::LOCAL_BME_Humidity);
			_plotManager.add(local_bme_humi_plot);

			box_bme_temp_plot = make_plot(IndicatorNames::BOX_BME_Temps);
			_plotManager.add(box_bme_temp_plot);
			box_bme_pres_plot = make_plot(IndicatorNames::BOX_BME_Pressure);
			_plotManager.add(box_bme_pres_plot);
			box_bme_humi_plot = make_plot(IndicatorNames::BOX_BME_Humidity);
			_plotManager.add(box_bme_humi_plot);

			sipm_plot = make_plot(IndicatorNames::SiPM_Plot);
			_plotManager.add(sipm_plot);
		}

		// No copying
		GUIManager(const GUIManager&) = delete;

		~GUIManager() { }

		void operator()() {
			// For std::get, we cannot use auto because it will go straight
			// to const T& which is not allowed with these non-copyable
			// types
			static TeensyInQueue& teensyQueue
				= std::get<TeensyInQueue&>(_queues);
			static auto teensyQueueFunc = [&](TeensyQueueInType&& f) -> bool {
				return teensyQueue.try_enqueue(f);
			};

			// end CAEN Controls

			static auto config = toml::parse_file("gui_setup.toml");
			static auto teensyConf = config["Teensy"];
			static std::string runDir
				= config["File"]["RunDir"].value_or("./RUNS");
			static std::string runName
				= config["File"]["RunName"].value_or("Testing");

			ImGui::Begin("Control Window");  

			if (ImGui::BeginTabBar("ControlTabs")) {

				if(ImGui::BeginTabItem("Run")) {

					ImGui::InputText("Run Directory", &runDir);
					ImGui::InputText("Run name", &runName);

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Teensy")) {

					// This makes all the items between PushItemWidth & PopItemWidth 80 px (?) wide
					ImGui::PushItemWidth(80);

					// 1.0 -> 0.5
					// 0.5 -> 1.0
					// (1.0 - 0.5) / (0.5 - 1.0) = -1
					static float connected_mod = 1.5;

					// Com port text input
					// We do not make this a GUI queue item because
					// we only need to let the client know when we click Connnect
					static std::string com_port = teensyConf["Port"].value_or("COM4");
					ImGui::InputText("Teensy COM port", &com_port);

					// This button starts the teensy communication
					ImGui::SameLine();
					// Colors to pop up or shadow it depending on the conditions
					ImGui::PushStyleColor(ImGuiCol_Button, 
						static_cast<ImVec4>(ImColor::HSV(2.0 / 7.0f, 0.6f, connected_mod*0.6f)));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 
						static_cast<ImVec4>(ImColor::HSV(2.0 / 7.0f, 0.7f, connected_mod*0.7f)));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, 
						static_cast<ImVec4>(ImColor::HSV(2.0 / 7.0f, 0.8f, connected_mod*0.8f)));
					// The operator () carries the ImGUI drawing functions
					// the function it needs is the callback
					if(connect_btn(teensyQueueFunc,
						[=](TeensyControllerState& oldState) {
							// We capture the com_value with connect on top of the button state
							// which in this case is not going to be used.
							oldState.Port = com_port;
							oldState.RunDir = runDir;
							oldState.RunName = runName;
							oldState.CurrentState
								= TeensyControllerStates::AttemptConnection;
							return true;
						}
					)) {
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

					if(disconnect_btn(teensyQueueFunc,
						// Teensy Stuff
						[=](TeensyControllerState& oldState) {
							// No need to disconnect if its not connected
							if(oldState.CurrentState == TeensyControllerStates::Connected) {
								oldState.CurrentState
									= TeensyControllerStates::Disconnected;
							}
							return true;
						}
					)) {
						// Local stuff
						connected_mod = 1.5;
					}

					ImGui::PopStyleColor(3);
					ImGui::PopItemWidth();

					pid_relay_chkBox(teensyQueueFunc, 
						ImGui::IsItemEdited,
						[=](TeensyControllerState& oldState) {
							oldState.PIDRelayState = pid_relay_chkBox.Get();
							return send_teensy_cmd(TeensyCommands::SetPIDRelay, oldState);
						}
					);

					gen_relay_chkBox(teensyQueueFunc,
						ImGui::IsItemEdited,
						[=](TeensyControllerState& oldState) {
							oldState.GeneralRelayState = gen_relay_chkBox.Get();
							return send_teensy_cmd(TeensyCommands::SetGeneralRelay, oldState);
						}
					);

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("PID1")) {

					pid_one_chkBox(teensyQueueFunc,
						ImGui::IsItemEdited,
						[=](TeensyControllerState& oldState) {
							bool state = pid_one_chkBox.Get();
							oldState.PIDOneState = state ? SBCQueens::PIDState::Running : SBCQueens::PIDState::Standby;
							return state ? send_teensy_cmd(TeensyCommands::StartPIDOne, oldState) : send_teensy_cmd(TeensyCommands::StopPIDOne, oldState);
						}
					);

					ImGui::Text("PID Setpoints");
					pid_one_temp_sp_inpF(teensyQueueFunc,
						0.01f, 6.0f, "%.6f °C", ImGui::IsItemDeactivated,
						[=](TeensyControllerState& oldState) {
							oldState.PIDOneTempValues.SetPoint = pid_one_temp_sp_inpF.Get();
							return send_teensy_cmd(TeensyCommands::SetPIDOneTempSetpoint, oldState);
						}
					);

					pid_one_curr_sp_inpF(teensyQueueFunc,
						0.01f, 6.0f, "%.6f A", ImGui::IsItemDeactivated,
						[=](TeensyControllerState& oldState) {
							oldState.PIDOneCurrentValues.SetPoint = pid_one_curr_sp_inpF.Get();
							return send_teensy_cmd(TeensyCommands::SetPIDOneCurrSetpoint, oldState);
						}
					);

					ImGui::Text("Temperature PID coefficients.");
					pid_one_PIDTKP_inpF(teensyQueueFunc,
						0.01f, 6.0f, "%.6f", ImGui::IsItemDeactivated,
						[=](TeensyControllerState& oldState) {
							oldState.PIDOneTempValues.Kp = pid_one_PIDTKP_inpF.Get();
							return send_teensy_cmd(TeensyCommands::SetPIDOneTempKp, oldState);
						}
					);
					pid_one_PIDTTI_inpF(teensyQueueFunc,
						0.01f, 6.0f, "%.6f ms", ImGui::IsItemDeactivated,
						[=](TeensyControllerState& oldState) {
							oldState.PIDOneTempValues.Ti = pid_one_PIDTTI_inpF.Get();
							return send_teensy_cmd(TeensyCommands::SetPIDOneTempTi, oldState);
						}
					);
					pid_one_PIDTTd_inpF(teensyQueueFunc,
						0.01f, 6.0f, "%.6f ms", ImGui::IsItemDeactivated,
						[=](TeensyControllerState& oldState) {
							oldState.PIDOneTempValues.Td = pid_one_PIDTTd_inpF.Get();
							return send_teensy_cmd(TeensyCommands::SetPIDOneTempTd, oldState);
						}
					);
	
					ImGui::Text("Current PID coefficients.");
					pid_one_PIDAKP_inpF(teensyQueueFunc,
						0.01f, 6.0f, "%.6f", ImGui::IsItemDeactivated,
						[=](TeensyControllerState& oldState) {
							oldState.PIDOneCurrentValues.Kp = pid_one_PIDAKP_inpF.Get();
							return send_teensy_cmd(TeensyCommands::SetPIDOneCurrKp, oldState);
						}
					);
					pid_one_PIDATI_inpF(teensyQueueFunc,
						0.01f, 6.0f, "%.6f ms", ImGui::IsItemDeactivated,
						[=](TeensyControllerState& oldState) {
							oldState.PIDOneCurrentValues.Ti = pid_one_PIDATI_inpF.Get();
							return send_teensy_cmd(TeensyCommands::SetPIDOneCurrTi, oldState);
						}
					);
					pid_one_PIDATD_inpF(teensyQueueFunc,
						0.01f, 6.0f, "%.6f ms", ImGui::IsItemDeactivated,
						[=](TeensyControllerState& oldState) {
							oldState.PIDOneCurrentValues.Td = pid_one_PIDATD_inpF.Get();
							return send_teensy_cmd(TeensyCommands::SetPIDOneCurrTd, oldState);
						}
					);

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("PID2")) {

					pid_two_chkBox(teensyQueueFunc,
						ImGui::IsItemEdited,
						[=](TeensyControllerState& oldState) {
							bool state = pid_two_chkBox.Get();
							oldState.PIDTwoState = state ? SBCQueens::PIDState::Running : SBCQueens::PIDState::Standby;
							return state ? send_teensy_cmd(TeensyCommands::StartPIDTwo, oldState) : send_teensy_cmd(TeensyCommands::StopPIDTwo, oldState);
						}
					);

					ImGui::Text("PID Setpoints");
					pid_two_temp_sp_inpF(teensyQueueFunc,
						0.01f, 6.0f, "%.6f °C", ImGui::IsItemDeactivated,
						[=](TeensyControllerState& oldState) {
							oldState.PIDTwoTempValues.SetPoint = pid_two_temp_sp_inpF.Get();
							return send_teensy_cmd(TeensyCommands::SetPIDTwoTempSetpoint, oldState);
						}
					);
					pid_two_curr_sp_inpF(teensyQueueFunc,
						0.01f, 6.0f, "%.6f A", ImGui::IsItemDeactivated,
						[=](TeensyControllerState& oldState) {
							oldState.PIDTwoCurrentValues.SetPoint = pid_two_curr_sp_inpF.Get();
							return send_teensy_cmd(TeensyCommands::SetPIDTwoCurrSetpoint, oldState);
						}
					);

					ImGui::Text("Temperature PID coefficients.");
					pid_two_PIDTKP_inpF(teensyQueueFunc,
						0.01f, 6.0f, "%.6f", ImGui::IsItemDeactivated,
						[=](TeensyControllerState& oldState) {
							oldState.PIDTwoTempValues.Kp = pid_two_PIDTKP_inpF.Get();
							return send_teensy_cmd(TeensyCommands::SetPIDTwoTempKp, oldState);
						}
					);
					pid_two_PIDTTI_inpF(teensyQueueFunc,
						0.01f, 6.0f, "%.6f ms", ImGui::IsItemDeactivated,
						[=](TeensyControllerState& oldState) {
							oldState.PIDTwoTempValues.Ti = pid_two_PIDTTI_inpF.Get();
							return send_teensy_cmd(TeensyCommands::SetPIDTwoTempTi, oldState);
						}
					);
					pid_two_PIDTTd_inpF(teensyQueueFunc,
						0.01f, 6.0f, "%.6f ms", ImGui::IsItemDeactivated,
						[=](TeensyControllerState& oldState) {
							oldState.PIDTwoTempValues.Td = pid_two_PIDTTd_inpF.Get();
							return send_teensy_cmd(TeensyCommands::SetPIDTwoTempTd, oldState);
						}
					);
	
					ImGui::Text("Current PID coefficients.");
					pid_two_PIDAKP_inpF(teensyQueueFunc,
						0.01f, 6.0f, "%.6f", ImGui::IsItemDeactivated,
						[=](TeensyControllerState& oldState) {
							oldState.PIDTwoCurrentValues.Kp = pid_two_PIDAKP_inpF.Get();
							return send_teensy_cmd(TeensyCommands::SetPIDTwoCurrKp, oldState);
						}
					);
					pid_two_PIDATI_inpF(teensyQueueFunc,
						0.01f, 6.0f, "%.6f ms", ImGui::IsItemDeactivated,
						[=](TeensyControllerState& oldState) {
							oldState.PIDTwoCurrentValues.Ti = pid_two_PIDATI_inpF.Get();
							return send_teensy_cmd(TeensyCommands::SetPIDTwoCurrTi, oldState);
						}
					);
					pid_two_PIDATD_inpF(teensyQueueFunc,
						0.01f, 6.0f, "%.6f ms", ImGui::IsItemDeactivated,
						[=](TeensyControllerState& oldState) {
							oldState.PIDTwoCurrentValues.Td = pid_two_PIDATD_inpF.Get();
							return send_teensy_cmd(TeensyCommands::SetPIDTwoCurrTd, oldState);
						}
					);


					ImGui::EndTabItem();
				}

				if(ImGui::BeginTabItem("CAEN")) {

								// CAEN Controls
					static CAENQueue& caenQueue = std::get<CAENQueue&>(_queues);
					static auto caenQueueFunc = [&](CAENQueueType&& f) -> bool {
						return caenQueue.try_enqueue(f);
					};

					ImGui::PushItemWidth(80);

					// 1.0 -> 0.5
					// 0.5 -> 1.0
					// (1.0 - 0.5) / (0.5 - 1.0) = -1
					static float connected_mod = 1.5;

					static auto CAENConf = config["CAEN"];
					static int maxEventsPerRead
						= CAENConf["MaxEventsPerRead"].value_or(512u);
					static int channel
						= CAENConf["Channel"].value_or(0u);
					static int offset
						= CAENConf["Offset"].value_or(0x8000u);;
					static int recordLength
						= CAENConf["RecordLength"].value_or(2048u);
					static int polarity
						= CAENConf["Polarity"].value_or(0u);
					static int range
						= CAENConf["Range"].value_or(0u);
					static int postbuffer
						= CAENConf["PostBufferPorcentage"].value_or(50u);
					static int threshold
						= CAENConf["Threshold"].value_or(0x8000u);


					static int caen_port = 0;
					ImGui::InputInt("CAEN port", &caen_port);
			        if (ImGui::IsItemHovered()) {
            			ImGui::SetTooltip("Usually 0 as long as there is no other"
            			"CAEN digitizers connected. If there is more, the numbers increase "
            			"as they were connected to the computer.");
			        }

					
					ImGui::SameLine();
					// Colors to pop up or shadow it depending on the conditions
					ImGui::PushStyleColor(ImGuiCol_Button, 
						static_cast<ImVec4>(ImColor::HSV(2.0 / 7.0f, 0.6f, connected_mod*0.6f)));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 
						static_cast<ImVec4>(ImColor::HSV(2.0 / 7.0f, 0.7f, connected_mod*0.7f)));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, 
						static_cast<ImVec4>(ImColor::HSV(2.0 / 7.0f, 0.8f, connected_mod*0.8f)));

					// This button starts the CAEN communication
					if(caen_connect_btn(caenQueueFunc,
						[=](CAENInterfaceState& state) {
							state.PortNum = caen_port;
							state.Config.Channel =
								static_cast<uint8_t>(channel);
							state.Config.MaxEventsPerRead =
								static_cast<uint32_t>(maxEventsPerRead);
							state.Config.DCOffset =
								static_cast<uint32_t>(offset);
							state.Config.RecordLength =
								static_cast<uint32_t>(recordLength);
							state.Config.DCRange =
								static_cast<uint8_t>(range);
							state.Config.TriggerThreshold =
								static_cast<uint32_t>(threshold);
							state.Config.TriggerPolarity =
								static_cast<CAEN_DGTZ_TriggerPolarity_t>(polarity);
							state.Config.PostTriggerPorcentage =
								static_cast<uint32_t>(postbuffer);
							state.CurrentState =
								CAENInterfaceStates::AttemptConnection;
							return true;
						}
					)) {
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
					
					if(caen_disconnect_btn(caenQueueFunc,
						[=](CAENInterfaceState& state) {

							// Only change the state if any of these states
							if(state.CurrentState == CAENInterfaceStates::OscilloscopeMode ||
								state.CurrentState == CAENInterfaceStates::StatisticsMode ||
								state.CurrentState == CAENInterfaceStates::RunMode){
									spdlog::info("Going to disconnect the CAEN");
									state.CurrentState = CAENInterfaceStates::Disconnected;
							}
							return true;
						}
					)) {
						// Local stuff
						connected_mod = 1.5;
					}

					ImGui::PopStyleColor(3);

					ImGui::SameLine();
					send_soft_trig(caenQueueFunc,
						[](CAENInterfaceState& state) {
							software_trigger(state.Port);
							return true;
						}
					);

					ImGui::PopItemWidth();

					ImGui::EndTabItem();


					ImGui::InputInt("Channel", &channel);

					ImGui::InputInt("MaxEventsPerRead", &maxEventsPerRead);
					ImGui::InputInt("Offset", &offset);
					
					ImGui::InputInt("Record Length", &recordLength);
        			ImGui::RadioButton("2V", &range, 0); ImGui::SameLine();
        			ImGui::RadioButton("0.5V", &range, 1);
        			ImGui::Text("Trigger Polarity:"); ImGui::SameLine();
        			ImGui::RadioButton("Positive", &polarity, 0); ImGui::SameLine();
        			ImGui::RadioButton("Negative", &polarity, 1);
        			ImGui::InputInt("Post-Trigger buffer %", &postbuffer);
        			ImGui::InputInt("Threshold", &threshold);

        			start_processing_btn(caenQueueFunc,
        				[](CAENInterfaceState& state) {
							if(state.CurrentState == CAENInterfaceStates::OscilloscopeMode ||
								state.CurrentState == CAENInterfaceStates::RunMode){
								state.CurrentState = CAENInterfaceStates::StatisticsMode;
							}
							return true;
						}
					);

					// (TODO: Hector) What other things do I need for CAEN?

		}

				ImGui::EndTabBar();
			} 

			ImGui::End();

			//// Plots
			ImGui::Begin("Teensy-BME280 Plots"); 

			// This functor updates the plots values from the queue.
			_plotManager();
			const auto g_axis_flags = ImPlotAxisFlags_AutoFit;
			// /// Teensy-BME280 Plots
			if(ImGui::Button("Clear")) {
				local_bme_humi_plot.clear();
				local_bme_temp_plot.clear();
				local_bme_pres_plot.clear();
				box_bme_humi_plot.clear();
				box_bme_temp_plot.clear();
				box_bme_pres_plot.clear();
			}
			if (ImGui::BeginTabBar("BME Plots")) {
				if (ImGui::BeginTabItem("Local BME")) {
					if (ImPlot::BeginPlot("Local BME", ImVec2(-1,0))) {

							// We setup the axis
							ImPlot::SetupAxes("time [s]", "Humidity [%]", g_axis_flags, g_axis_flags);
							ImPlot::SetupAxis(ImAxis_Y3, "Pressure [Pa]", g_axis_flags | ImPlotAxisFlags_Opposite);
							ImPlot::SetupAxis(ImAxis_Y2, "Temperature [degC]", g_axis_flags | ImPlotAxisFlags_Opposite);

							// This one does not need an SetAxes as it takes the default
							// This functor is almost the same as calling ImPlot
							local_bme_humi_plot("Humidity");

							// We need to call SetAxes before ImPlot::PlotLines to let the API know
							// the axis of our data
							ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
							local_bme_temp_plot("Temperature");

							ImPlot::SetAxes(ImAxis_X1, ImAxis_Y3);
							local_bme_pres_plot("Pressure");

						ImPlot::EndPlot();
					}
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Box BME")) {
					if (ImPlot::BeginPlot("Local BME", ImVec2(-1,0))) {

						// We setup the axis
						ImPlot::SetupAxes("time [s]", "Humidity [%]", g_axis_flags, g_axis_flags);
						ImPlot::SetupAxis(ImAxis_Y3, "Pressure [Pa]", g_axis_flags | ImPlotAxisFlags_Opposite);
						ImPlot::SetupAxis(ImAxis_Y2, "Temperature [degC]", g_axis_flags | ImPlotAxisFlags_Opposite);

						// This one does not need an SetAxes as it takes the default
						// This functor is almost the same as calling ImPlot
						box_bme_humi_plot("Humidity");

						// We need to call SetAxes before ImPlot::PlotLines to let the API know
						// the axis of our data
						ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
						box_bme_temp_plot("Temperature");

						ImPlot::SetAxes(ImAxis_X1, ImAxis_Y3);
						box_bme_pres_plot("Pressure");
							
						ImPlot::EndPlot();
					}

					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}

			ImGui::End();
			// /// !Teensy-BME280 Plots
			// ////
			// /// Teensy-PID Plots
			ImGui::Begin("Teensy-PID Plots"); 
			if(ImGui::Button("Clear")) {
					pid1_temps_plot.clear();
					pid1_currs_plot.clear();
					pid2_temps_plot.clear();
					pid2_currs_plot.clear();
			}
			if (ImPlot::BeginPlot("PIDs", ImVec2(-1,0))) {
				ImPlot::SetupAxes("time [s]", "Temperature [degC]", g_axis_flags, g_axis_flags);
				ImPlot::SetupAxis(ImAxis_Y2, "Current [A]", g_axis_flags | ImPlotAxisFlags_Opposite);

				pid1_temps_plot("RTD1");

				ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
				pid1_currs_plot("Driver 1");

				ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
				pid2_temps_plot("RTD2");

				ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
				pid2_currs_plot("Driver 2");

				ImPlot::EndPlot();
			}

			ImGui::End();
			/// !Teensy-PID Plots
			////
			/// SiPM Plots

			sipm_plot.ClearOnNewData = true;
			ImGui::Begin("SiPM Plot");  
			if (ImPlot::BeginPlot("SiPM Trace", ImVec2(-1,0))) {

				ImPlot::SetupAxes("time [ns]", "Counts", g_axis_flags, g_axis_flags);

				sipm_plot("SiPM");

				ImPlot::EndPlot();
			}
			ImGui::End();
			/// !SiPM Plots
			//// !Plots

			ImGui::Begin("Indicators");

			// Teensy
			ImGui::Text("Teensy");
			ImGui::Text("Latest PID1 Temp"); ImGui::SameLine(); latest_pid1_temp(); ImGui::SameLine(); ImGui::Text("[degC]");
			ImGui::Text("Latest PID1 Curr"); ImGui::SameLine(); latest_pid1_curr(); ImGui::SameLine(); ImGui::Text("[A]");
			ImGui::Text("Latest PID2 Temp"); ImGui::SameLine(); latest_pid2_temp(); ImGui::SameLine(); ImGui::Text("[degC]");
			ImGui::Text("Latest PID2 Curr"); ImGui::SameLine(); latest_pid2_curr(); ImGui::SameLine(); ImGui::Text("[A]");

			ImGui::Text("Latest BOX BME Humidity"); ImGui::SameLine(); latest_box_bme_hum(); ImGui::SameLine(); ImGui::Text("[%%]");
			ImGui::Text("Latest BOX BME Temperature"); ImGui::SameLine(); latest_box_bme_temp(); ImGui::SameLine(); ImGui::Text("[degC]");
			// End Teensy

			// CAEN
			ImGui::Text("SiPM Statistics");
			ImGui::Text("Events in buffer"); ImGui::SameLine(); eventsBuffer(); ImGui::SameLine(); ImGui::Text("Counts");
			ImGui::Text("Signal frequency"); ImGui::SameLine(); frequency(); ImGui::SameLine(); ImGui::Text("Hz");
			ImGui::Text("Dark noise rate"); ImGui::SameLine(); dark_noise(); ImGui::SameLine(); ImGui::Text("Hz");
			ImGui::Text("Gain"); ImGui::SameLine(); gain(); ImGui::SameLine(); ImGui::Text("[counts x ns]");
			// End CAEN
			ImGui::End();
		}

		// closing is called when the GUI is manually closed.
		void closing() {
			// The only action to take is that we let the 
			// Teensy Thread to close, too.
			TeensyInQueue& teensyQueue = std::get<TeensyInQueue&>(_queues);
		 	teensyQueue.try_enqueue(
		 		[](TeensyControllerState& oldState) {
					oldState.CurrentState = TeensyControllerStates::Closing;
					return true;
				}
			);

			CAENQueue& caenQueue = std::get<CAENQueue&>(_queues);
			caenQueue.try_enqueue(
				[](CAENInterfaceState& state) {
					state.CurrentState = CAENInterfaceStates::Closing;
					return true;
				}
			);
			

		}

	};

} // namespace SBCQueens