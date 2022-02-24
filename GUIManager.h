#pragma once

#include <cstdint>
#include <math.h>
#include <memory>
#include <string>
#include <unordered_map>
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
#include "caen_helper.h"
#include "imgui_helpers.h"
#include "implot_helpers.h"
#include "indicators.h"

//TODO(Hector): fix and clean this mess of a code!

#define MAX_CHANNELS 64

namespace SBCQueens {

	template<typename... QueueFuncs>
	class GUIManager
	{
private:

		std::tuple<QueueFuncs&...> _queues;
		toml::table _toml_file;

		IndicatorReceiver<IndicatorNames> _indicatorReceiver;

		// Controls
		ControlLink<TeensyInQueue> TeensyControlFac;
		TeensyControllerState tgui_state;

		// // CAEN Controls
		ControlLink<CAENQueue> CAENControlFac;
		CAENInterfaceState cgui_state;

		std::string i_run_dir;
		std::string i_run_name;

		toml::table config_file;

public:
		explicit GUIManager(QueueFuncs&... queues) : 
			_queues(forward_as_tuple(queues...)),
			_indicatorReceiver 	(std::get<SiPMsPlotQueue&>(_queues)),
			TeensyControlFac 	(std::get<TeensyInQueue&>(_queues)),
			CAENControlFac 		(std::get<CAENQueue&>(_queues)) {

			// Controls
			// When config_file goes out of scope, everything
			// including the daughters get cleared
			config_file = toml::parse_file("gui_setup.toml");
			auto t_conf = config_file["Teensy"];
			auto CAEN_conf = config_file["CAEN"];
			auto file_conf = config_file["File"];

			// These two guys are shared between CAEN and Teensy
			i_run_dir 		= config_file["File"]["RunDir"].value_or("./RUNS");
			i_run_name 		= config_file["File"]["RunName"].value_or("Testing");

			// Teensy initial state
			tgui_state.CurrentState = TeensyControllerStates::Standby;
			tgui_state.Port 		= t_conf["Port"].value_or("COM4");
			tgui_state.RunDir 		= i_run_dir;
			tgui_state.RunName 		= i_run_name;

			// Send initial states of the PIDs
			tgui_state.GeneralRelayState = false;
			tgui_state.PIDRelayState = false;

			tgui_state.PIDOneState = PIDState::Standby;
			tgui_state.PIDOneTempValues.SetPoint = t_conf["PID1TempSetpoint"].value_or(0.0f);
			tgui_state.PIDOneTempValues.Kp = t_conf["PID1TKp"].value_or(0.0f);
			tgui_state.PIDOneTempValues.Ti = t_conf["PID1TTi"].value_or(0.0f);
			tgui_state.PIDOneTempValues.Td = t_conf["PID1TTd"].value_or(0.0f);

			tgui_state.PIDOneCurrentValues.SetPoint = t_conf["PID1CurrentSetpoint"].value_or(0.0f);
			tgui_state.PIDOneCurrentValues.Kp = t_conf["PID1AKp"].value_or(0.0f);
			tgui_state.PIDOneCurrentValues.Ti = t_conf["PID1ATi"].value_or(0.0f);
			tgui_state.PIDOneCurrentValues.Td = t_conf["PID2ATd"].value_or(0.0f);

			//PID2Enable
			tgui_state.PIDTwoState = PIDState::Standby;
			tgui_state.PIDTwoTempValues.SetPoint = t_conf["PID2TempSetpoint"].value_or(0.0f);
			tgui_state.PIDTwoTempValues.Kp = t_conf["PID2TKp"].value_or(0.0f);
			tgui_state.PIDTwoTempValues.Ti = t_conf["PID2TTi"].value_or(0.0f);
			tgui_state.PIDTwoTempValues.Td = t_conf["PID2TTd"].value_or(0.0f);

			tgui_state.PIDTwoCurrentValues.SetPoint = t_conf["PID2CurrentSetpoint"].value_or(0.0f);
			tgui_state.PIDTwoCurrentValues.Kp = t_conf["PID2AKp"].value_or(0.0f);
			tgui_state.PIDTwoCurrentValues.Ti = t_conf["PID2ATi"].value_or(0.0f);
			tgui_state.PIDTwoCurrentValues.Td = t_conf["PID2ATd"].value_or(0.0f);

			// CAEN initial state
			cgui_state.CurrentState = CAENInterfaceStates::Standby;
			cgui_state.RunDir = i_run_dir;
			cgui_state.RunName = i_run_name;
			cgui_state.SiPMParameters = file_conf["SiPMParameters"].value_or("default");

			cgui_state.Model = CAENDigitizerModels_map.at(CAEN_conf["Model"].value_or("DT5730B"));
			cgui_state.PortNum = CAEN_conf["Port"].value_or(0u);

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
			for(uint8_t ch = 0; ch < MAX_CHANNELS; ch++) {
				std::string ch_toml = "group" + std::to_string(ch);
				if(auto ch_conf = CAEN_conf[ch_toml].as_table()) {

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
							// Max number of channels there can be is 8
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
		}

		// No copying
		GUIManager(const GUIManager&) = delete;

		~GUIManager() { }

		void operator()() {

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

					// This is the only button that does send a task to all
					// (so far) threads.
					// TODO(Hector): generalize this in the future
					CAENControlFac.Button(
						"Start Data Taking",
						[=](CAENInterfaceState& state) {
						// Only change state if its in a work related
						// state, i.e oscilloscope mode
						if(state.CurrentState == CAENInterfaceStates::OscilloscopeMode ||
							state.CurrentState == CAENInterfaceStates::StatisticsMode) {
							state.CurrentState = CAENInterfaceStates::RunMode;
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

				teensy_tabs();
				caen_tabs();
				ImGui::EndTabBar();
			} 

			ImGui::End();

			//// Plots
			ImGui::Begin("Teensy-BME280 Plots"); 

			// This functor updates the plots values from the queue.
			_indicatorReceiver();

			const auto g_axis_flags = ImPlotAxisFlags_AutoFit;
			// /// Teensy-BME280 Plots
			if(ImGui::Button("Clear")) {
				// local_bme_humi_plot.clear();
				// local_bme_temp_plot.clear();
				// local_bme_pres_plot.clear();
				// box_bme_humi_plot.clear();
				// box_bme_temp_plot.clear();
				// box_bme_pres_plot.clear();
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
						_indicatorReceiver.plot(IndicatorNames::LOCAL_BME_Humidity, "Humidity");

						// We need to call SetAxes before ImPlot::PlotLines to let the API know
						// the axis of our data
						ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
						_indicatorReceiver.plot(IndicatorNames::LOCAL_BME_Temps, "Temperature");

						ImPlot::SetAxes(ImAxis_X1, ImAxis_Y3);
						_indicatorReceiver.plot(IndicatorNames::LOCAL_BME_Pressure, "Pressure");

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
						_indicatorReceiver.plot(IndicatorNames::BOX_BME_Humidity, "Humidity");

						// We need to call SetAxes before ImPlot::PlotLines to let the API know
						// the axis of our data
						ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
						_indicatorReceiver.plot(IndicatorNames::BOX_BME_Temps, "Temperature");

						ImPlot::SetAxes(ImAxis_X1, ImAxis_Y3);
						_indicatorReceiver.plot(IndicatorNames::BOX_BME_Pressure, "Pressure");
							
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
					// pid1_temps_plot.clear();
					// pid1_currs_plot.clear();
					// pid2_temps_plot.clear();
					// pid2_currs_plot.clear();
			}
			if (ImPlot::BeginPlot("PIDs", ImVec2(-1,0))) {
				ImPlot::SetupAxes("time [s]", "Temperature [degC]", g_axis_flags, g_axis_flags);
				ImPlot::SetupAxis(ImAxis_Y2, "Current [A]", g_axis_flags | ImPlotAxisFlags_Opposite);

				_indicatorReceiver.plot(IndicatorNames::PID1_Temps, "RTD1");

				ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
				_indicatorReceiver.plot(IndicatorNames::PID1_Currs, "Driver 1");

				ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
				_indicatorReceiver.plot(IndicatorNames::PID2_Temps, "RTD2");

				ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
				_indicatorReceiver.plot(IndicatorNames::PID2_Currs, "Driver 2");

				ImPlot::EndPlot();
			}

			ImGui::End();
			/// !Teensy-PID Plots
			////
			/// SiPM Plots


			ImGui::Begin("SiPM Plot");  
			if (ImPlot::BeginPlot("SiPM Trace", ImVec2(-1,0))) {

				ImPlot::SetupAxes("time [ns]", "Counts", g_axis_flags, g_axis_flags);

				_indicatorReceiver.plot(IndicatorNames::SiPM_Plot_ZERO, "Plot 1", true);
				_indicatorReceiver.plot(IndicatorNames::SiPM_Plot_ONE, "Plot 2", true);
				_indicatorReceiver.plot(IndicatorNames::SiPM_Plot_TWO, "Plot 3", true);
				_indicatorReceiver.plot(IndicatorNames::SiPM_Plot_THREE, "Plot 4", true);

				ImPlot::EndPlot();
			}
			ImGui::End();
			/// !SiPM Plots
			//// !Plots

			ImGui::Begin("Indicators");

			// Teensy
			ImGui::Text("Teensy");
			_indicatorReceiver.indicator(IndicatorNames::LATEST_PID1_TEMP, "Latest PID1 Temp"); ImGui::SameLine(); ImGui::Text("[degC]");
			_indicatorReceiver.indicator(IndicatorNames::LATEST_PID1_CURR, "Latest PID1 Curr"); ImGui::SameLine(); ImGui::Text("[A]");
			_indicatorReceiver.indicator(IndicatorNames::LATEST_PID2_TEMP, "Latest PID2 Temp"); ImGui::SameLine(); ImGui::Text("[degC]");
			_indicatorReceiver.indicator(IndicatorNames::LATEST_PID2_CURR, "Latest PID2 Curr"); ImGui::SameLine(); ImGui::Text("[A]");


			_indicatorReceiver.indicator(IndicatorNames::LATEST_BOX_BME_HUM, "Latest BOX BME Humidity"); ImGui::SameLine();ImGui::Text("[%%]");
			_indicatorReceiver.indicator(IndicatorNames::LATEST_BOX_BME_TEMP, "Latest BOX BME Temperature"); ImGui::SameLine(); ImGui::Text("[degC]");
			// End Teensy

			// CAEN
			ImGui::Text("SiPM Statistics");

			_indicatorReceiver.indicator(IndicatorNames::CAENBUFFEREVENTS, "Events in buffer", 4);
			ImGui::SameLine(); ImGui::Text("Counts");
			_indicatorReceiver.indicator(IndicatorNames::FREQUENCY, "Signal frequency", 4, NumericFormat::Scientific);
			ImGui::SameLine(); ImGui::Text("Hz");
			_indicatorReceiver.indicator(IndicatorNames::DARK_NOISE_RATE, "Dark noise rate", 3, NumericFormat::Scientific);
			ImGui::SameLine(); ImGui::Text("Hz");
			_indicatorReceiver.indicator(IndicatorNames::GAIN, "Gain", 3, NumericFormat::Scientific);
			ImGui::SameLine(); ImGui::Text("[counts x ns]");
			// End CAEN
			ImGui::End();
		}

		// closing is called when the GUI is manually closed.
		void closing() {
			// The only action to take is that we let the 
			// Teensy Thread to close, too.
			TeensyInQueue& tq = std::get<TeensyInQueue&>(_queues);
		 	tq.try_enqueue(
		 		[](TeensyControllerState& oldState) {
					oldState.CurrentState = TeensyControllerStates::Closing;
					return true;
				}
			);

		 	CAENQueue& cq = std::get<CAENQueue&>(_queues);
			cq.try_enqueue(
				[](CAENInterfaceState& state) {
					state.CurrentState = CAENInterfaceStates::Closing;
					return true;
				}
			);
		}

		void caen_tabs() {

			if(ImGui::BeginTabItem("CAEN Global")) {

				ImGui::PushItemWidth(80);

				// 1.0 -> 0.5
				// 0.5 -> 1.0
				// (1.0 - 0.5) / (0.5 - 1.0) = -1
				static float connected_mod = 1.5;

				CAENControlFac.ComboBox("Model", cgui_state.Model,
					CAENDigitizerModels_map, [](){ return false; }, [](){ return true; });

				static int caen_port = 0;
				ImGui::InputInt("CAEN port", &caen_port);
		        if (ImGui::IsItemHovered()) {
        			ImGui::SetTooltip("Usually 0 as long as there is no "
        			"other CAEN digitizers connected. If there are more, "
        			"the port increases as they were connected to the "
        			"computer.");
		        }

				ImGui::SameLine();
				// Colors to pop up or shadow it depending on the conditions
				ImGui::PushStyleColor(ImGuiCol_Button,
					static_cast<ImVec4>(ImColor::HSV(2.0 / 7.0f, 0.6f, connected_mod*0.6f)));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
					static_cast<ImVec4>(ImColor::HSV(2.0 / 7.0f, 0.7f, connected_mod*0.7f)));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive,
					static_cast<ImVec4>(ImColor::HSV(2.0 / 7.0f, 0.8f, connected_mod*0.8f)));

				// This button starts the CAEN communication and sends all
				// the setup configuration
				if(CAENControlFac.Button("Connect",
					[=](CAENInterfaceState& state) {

						state = cgui_state;

						state.RunDir = i_run_dir;
						state.RunName = i_run_name;
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

				if(CAENControlFac.Button("Disconnect",
					[=](CAENInterfaceState& state) {

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
					connected_mod = 1.5;
				}

				ImGui::PopStyleColor(3);

				ImGui::PopItemWidth();

				ImGui::InputScalar("Max Events Per Read", ImGuiDataType_U32,
					&cgui_state.GlobalConfig.MaxEventsPerRead);

				ImGui::InputScalar("Record Length [counts]", ImGuiDataType_U32,
					&cgui_state.GlobalConfig.RecordLength);
				ImGui::InputScalar("Post-Trigger buffer %", ImGuiDataType_U32,
					&cgui_state.GlobalConfig.PostTriggerPorcentage);

    			ImGui::Checkbox("Overlapping Rejection",
    				&cgui_state.GlobalConfig.TriggerOverlappingEn);
    			if(ImGui::IsItemHovered()) {
    				ImGui::SetTooltip("If checked, overlapping rejection "
    					"is disabled which leads to a non-constant record length. "
    					"HIGHLY UNSTABLE FEATURE, DO NOT ENABLE.");
    			}

    			ImGui::Checkbox("TRG-IN as Gate", &cgui_state.GlobalConfig.EXTasGate);

    			const std::unordered_map<CAEN_DGTZ_TriggerMode_t, std::string> tgg_mode_map = {
					{CAEN_DGTZ_TriggerMode_t::CAEN_DGTZ_TRGMODE_DISABLED, "Disabled"},
    				{CAEN_DGTZ_TriggerMode_t::CAEN_DGTZ_TRGMODE_ACQ_ONLY, "Acq Only"},
    				{CAEN_DGTZ_TriggerMode_t::CAEN_DGTZ_TRGMODE_EXTOUT_ONLY, "Ext Only"},
    				{CAEN_DGTZ_TriggerMode_t::CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT, "Both"}};

    			CAENControlFac.ComboBox("External Trigger Mode",
    				cgui_state.GlobalConfig.EXTTriggerMode,
					tgg_mode_map,
    				[]() {return false;}, [](){});

    			CAENControlFac.ComboBox("Software Trigger Mode",
    				cgui_state.GlobalConfig.SWTriggerMode,
    				tgg_mode_map,
    				[]() {return false;}, [](){});

    			CAENControlFac.ComboBox("Trigger Polarity",
    				cgui_state.GlobalConfig.TriggerPolarity,
    				{{CAEN_DGTZ_TriggerPolarity_t::CAEN_DGTZ_TriggerOnFallingEdge, "Falling Edge"},
    				{CAEN_DGTZ_TriggerPolarity_t::CAEN_DGTZ_TriggerOnRisingEdge, "Rising Edge"}},
    				[]() {return false;}, [](){});

    			CAENControlFac.ComboBox("IO Level",
    				cgui_state.GlobalConfig.IOLevel,
    				{{CAEN_DGTZ_IOLevel_t::CAEN_DGTZ_IOLevel_NIM, "NIM"},
    				{CAEN_DGTZ_IOLevel_t::CAEN_DGTZ_IOLevel_TTL, "TTL"}},
    				[]() {return false;}, [](){});

				CAENControlFac.Button("Software Trigger",
					[](CAENInterfaceState& state) {
						//software_trigger(state.Port);
						return true;
					}
				);
				if(ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Forces a trigger in the digitizer if "
						"the feature is enabled");
				}

    			CAENControlFac.Button("Start processing",
    				[](CAENInterfaceState& state) {
						if(state.CurrentState == CAENInterfaceStates::OscilloscopeMode ||
							state.CurrentState == CAENInterfaceStates::RunMode){
							state.CurrentState = CAENInterfaceStates::StatisticsMode;
						}
						return true;
					}
				);
				if(ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Starts measuring pulses and processing "
						"them without saving to file. Intended for diagnostics.");
				}

    			ImGui::EndTabItem();
			}

			// Channel tabs
			for(auto& ch : cgui_state.GroupConfigs) {
				std::string tab_name = "CAEN GR" + std::to_string(ch.Number);

				if(ImGui::BeginTabItem(tab_name.c_str())) {
					ImGui::PushItemWidth(120);
					// ImGui::Checkbox("Enabled", ch.);
					ImGui::InputScalar("DC Offset [counts]", ImGuiDataType_U16,
						&ch.DCOffset);

    				ImGui::InputScalar("Threshold [counts]", ImGuiDataType_U16,
	    				&ch.TriggerThreshold);

					ImGui::InputScalar("Trigger Mask", ImGuiDataType_U8, 
						&ch.TriggerMask);
					ImGui::InputScalar("Acquisition Mask", ImGuiDataType_U8, 
						&ch.AcquisitionMask);

					int corr_i = 0;
					for(auto& corr : ch.DCCorrections) {
						std::string c_name = "Correction " + std::to_string(corr_i);
						ImGui::InputScalar(c_name.c_str(), ImGuiDataType_U8, &corr);
						corr_i++;
					}

					// TODO(Hector): change this to get the actual
					// ranges of the digitizer used, maybe change it to a dropbox?
	    			ImGui::RadioButton("2V",
	    			 	reinterpret_cast<int*>(&ch.DCRange), 0); ImGui::SameLine();
	    			ImGui::RadioButton("0.5V",
	    				reinterpret_cast<int*>(&ch.DCRange), 1);

	    			ImGui::EndTabItem();
				}

			}

		}

		void teensy_tabs() {
			// Actual IMGUI code now
			if (ImGui::BeginTabItem("Teensy")) {

				// 1.0 -> 0.5
				// 0.5 -> 1.0
				// (1.0 - 0.5) / (0.5 - 1.0) = -1
				static float connected_mod = 1.5;
				// This makes all the items between PushItemWidth
				// & PopItemWidth 80 px (?) wide
				ImGui::PushItemWidth(80);
				// Com port text input
				// We do not make this a GUI queue item because
				// we only need to let the client know when we click connnect
				ImGui::InputText("Teensy COM port", &tgui_state.Port);

				ImGui::SameLine();
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
				if(TeensyControlFac.Button("Connect",
					// To make things secure, we pass everything by value
					[=](TeensyControllerState& oldState) {
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

				if(TeensyControlFac.Button("Disconnect",
					[=](TeensyControllerState& oldState) {
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
				ImGui::PopItemWidth();

				TeensyControlFac.Checkbox("PID Relay",
					tgui_state.PIDRelayState,
					ImGui::IsItemEdited,
					[=](TeensyControllerState& oldState) {
						oldState.CommandToSend = TeensyCommands::SetPIDRelay;
						oldState.PIDRelayState = tgui_state.PIDRelayState;
						return true;
					}
				);

				TeensyControlFac.Checkbox("General Relay",
					tgui_state.GeneralRelayState,
					ImGui::IsItemEdited,
					[=](TeensyControllerState& oldState) {
						oldState.CommandToSend = TeensyCommands::SetGeneralRelay;
						oldState.GeneralRelayState = tgui_state.GeneralRelayState;
						return true;
					}
				);

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("PID1")) {
				ImGui::PushItemWidth(180);
				TeensyControlFac.ComboBox("PID1 State",
					tgui_state.PIDOneState,
					{{PIDState::Running, "Running"}, {PIDState::Standby, "Standby"}},
					ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {

						oldState.PIDOneState = tgui_state.PIDOneState;
						if(oldState.PIDOneState == PIDState::Standby) {
							oldState.CommandToSend = TeensyCommands::StopPIDOne;
						} else {
							oldState.CommandToSend = TeensyCommands::StartPIDOne;
						}

						return true;
					}
				);

				ImGui::Text("PID Setpoints");
				TeensyControlFac.InputFloat("Temperature Setpoint",
					tgui_state.PIDOneTempValues.SetPoint,
					0.01f, 6.0f, "%.6f °C",
					ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.CommandToSend = TeensyCommands::SetPIDOneTempSetpoint;
						oldState.PIDOneTempValues.SetPoint = tgui_state.PIDOneTempValues.SetPoint;
						return true;
					}
				);

				TeensyControlFac.InputFloat("Current Setpoint",
					tgui_state.PIDOneCurrentValues.SetPoint,
					0.01f, 6.0f, "%.6f A",
					ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.CommandToSend = TeensyCommands::SetPIDOneCurrSetpoint;
						oldState.PIDOneCurrentValues.SetPoint = tgui_state.PIDOneCurrentValues.SetPoint;
						return true;
					}
				);

				ImGui::Text("Temperature PID coefficients.");
				TeensyControlFac.InputFloat("Kp",
					tgui_state.PIDOneTempValues.Kp,
					0.01f, 6.0f, "%.6f",
					ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.CommandToSend = TeensyCommands::SetPIDOneTempKp;
						oldState.PIDOneTempValues.Kp = tgui_state.PIDOneTempValues.Kp;
						return true;
					}
				);
				TeensyControlFac.InputFloat("Ti",
					tgui_state.PIDOneTempValues.Ti,
					0.01f, 6.0f, "%.6f ms", ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.CommandToSend = TeensyCommands::SetPIDOneCurrSetpoint;
						oldState.PIDOneTempValues.Ti = tgui_state.PIDOneTempValues.Ti;
						return true;
					}
				);
				TeensyControlFac.InputFloat("Td",
					tgui_state.PIDOneTempValues.Td,
					0.01f, 6.0f, "%.6f ms",
					ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.CommandToSend = TeensyCommands::SetPIDOneTempTd;
						oldState.PIDOneTempValues.Td = tgui_state.PIDOneTempValues.Td;
						return true;
					}
				);

				ImGui::Text("Current PID coefficients.");
				TeensyControlFac.InputFloat("AKp",
					tgui_state.PIDOneCurrentValues.Kp,
					0.01f, 6.0f, "%.6f",
					ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.CommandToSend = TeensyCommands::SetPIDOneCurrKp;
						oldState.PIDOneCurrentValues.Kp = tgui_state.PIDOneCurrentValues.Kp;
						return true;
					}
				);
				TeensyControlFac.InputFloat("ATi",
					tgui_state.PIDOneCurrentValues.Ti,
					0.01f, 6.0f, "%.6f ms",
					ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.CommandToSend = TeensyCommands::SetPIDOneCurrTi;
						oldState.PIDOneCurrentValues.Ti = tgui_state.PIDOneCurrentValues.Ti;
						return true;
					}
				);
				TeensyControlFac.InputFloat("ATd",
					tgui_state.PIDOneCurrentValues.Td,
					0.01f, 6.0f, "%.6f ms",
					ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.CommandToSend = TeensyCommands::SetPIDOneCurrTd;
						oldState.PIDOneCurrentValues.Td = tgui_state.PIDOneCurrentValues.Td;
						return true;
					}
				);

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("PID2")) {
				ImGui::PushItemWidth(180);
				TeensyControlFac.ComboBox("PID2 State",
					tgui_state.PIDTwoState,
					{{PIDState::Running, "Running"}, {PIDState::Standby, "Standby"}},
					ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {

						oldState.PIDTwoState = tgui_state.PIDTwoState;
						if(oldState.PIDOneState == PIDState::Standby) {
							oldState.CommandToSend = TeensyCommands::StopPIDTwo;
						} else {
							oldState.CommandToSend = TeensyCommands::StartPIDTwo;
						}

						return true;
					}
				);

				ImGui::Text("PID Setpoints");
				TeensyControlFac.InputFloat("Temperature Setpoint",
					tgui_state.PIDTwoTempValues.SetPoint,
					0.01f, 6.0f, "%.6f °C",
					ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.CommandToSend = TeensyCommands::SetPIDTwoTempSetpoint;
						oldState.PIDTwoTempValues.SetPoint = tgui_state.PIDTwoTempValues.SetPoint;
						return true;
					}
				);

				TeensyControlFac.InputFloat("Current Setpoint",
					tgui_state.PIDTwoCurrentValues.SetPoint,
					0.01f, 6.0f, "%.6f A",
					ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.CommandToSend = TeensyCommands::SetPIDTwoCurrSetpoint;
						oldState.PIDTwoCurrentValues.SetPoint = tgui_state.PIDTwoCurrentValues.SetPoint;
						return true;
					}
				);

				ImGui::Text("Temperature PID coefficients.");
				TeensyControlFac.InputFloat("Kp",
					tgui_state.PIDTwoTempValues.Kp,
					0.01f, 6.0f, "%.6f",
					ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.CommandToSend = TeensyCommands::SetPIDTwoTempKp;
						oldState.PIDTwoTempValues.Kp = tgui_state.PIDTwoTempValues.Kp;
						return true;
					}
				);
				TeensyControlFac.InputFloat("Ti",
					tgui_state.PIDTwoTempValues.Ti,
					0.01f, 6.0f, "%.6f ms",
					ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.CommandToSend = TeensyCommands::SetPIDTwoTempTi;
						oldState.PIDTwoTempValues.Ti = tgui_state.PIDTwoTempValues.Ti;
						return true;
					}
				);
				TeensyControlFac.InputFloat("Td",
					tgui_state.PIDTwoTempValues.Td,
					0.01f, 6.0f, "%.6f ms",
					ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.CommandToSend = TeensyCommands::SetPIDTwoTempTd;
						oldState.PIDTwoTempValues.Td = tgui_state.PIDTwoTempValues.Td;
						return true;
					}
				);

				ImGui::Text("Current PID coefficients.");
				TeensyControlFac.InputFloat("Kp",
					tgui_state.PIDTwoCurrentValues.Kp,
					0.01f, 6.0f, "%.6f",
					ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.CommandToSend = TeensyCommands::SetPIDTwoCurrKp;
						oldState.PIDTwoCurrentValues.Kp = tgui_state.PIDTwoCurrentValues.Kp;
						return true;
					}
				);
				TeensyControlFac.InputFloat("ATi",
					tgui_state.PIDTwoCurrentValues.Ti,
					0.01f, 6.0f, "%.6f ms",
					ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.CommandToSend = TeensyCommands::SetPIDTwoCurrTi;
						oldState.PIDTwoCurrentValues.Ti = tgui_state.PIDTwoCurrentValues.Ti;
						return true;
					}
				);
				TeensyControlFac.InputFloat("ATd",
					tgui_state.PIDTwoCurrentValues.Td,
					0.01f, 6.0f, "%.6f ms",
					ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.CommandToSend = TeensyCommands::SetPIDTwoCurrTd;
						oldState.PIDTwoCurrentValues.Td = tgui_state.PIDTwoCurrentValues.Td;
						return true;
					}
				);


				ImGui::EndTabItem();
			}
		}

	};

} // namespace SBCQueens
