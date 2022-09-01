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
#include "OtherDevicesInterface.h"
#include "caen_helper.h"
#include "deps/imgui/imgui.h"
#include "deps/implot/implot.h"
#include "imgui_helpers.h"
#include "implot_helpers.h"
#include "indicators.h"

#include "GUI/IndicatorWindow.h"
#include "GUI/ControlWindow.h"

namespace SBCQueens {

	template<typename... QueueFuncs>
	class GUIManager
	{
private:

		std::tuple<QueueFuncs&...> _queues;

		IndicatorReceiver<IndicatorNames> _indicatorReceiver;

		// Teensy Controls
		ControlLink<TeensyInQueue> TeensyControlFac;
		TeensyControllerState tgui_state;

		// CAEN Controls
		ControlLink<CAENQueue> CAENControlFac;
		CAENInterfaceData cgui_state;

		// Other controls

		OtherDevicesData other_state;

		std::string i_run_dir;
		std::string i_run_name;

		toml::table config_file;

		IndicatorWindow indicatorWindow;
		ControlWindow controlWindow;

public:
		explicit GUIManager(QueueFuncs&... queues) : 
			_queues(forward_as_tuple(queues...)),
			_indicatorReceiver 	(std::get<SiPMsPlotQueue&>(_queues)),
			TeensyControlFac 	(std::get<TeensyInQueue&>(_queues)),
			CAENControlFac 		(std::get<CAENQueue&>(_queues)),
			indicatorWindow 	(_indicatorReceiver),
			controlWindow(TeensyControlFac, tgui_state, CAENControlFac, cgui_state, other_state) {

			// Controls
			// When config_file goes out of scope, everything
			// including the daughters get cleared
			// config_file = toml::parse_file("gui_setup.toml");
			// auto t_conf = config_file["Teensy"];
			// auto CAEN_conf = config_file["CAEN"];
			// auto file_conf = config_file["File"];

			// indicatorWindow.init(config_file);
			// controlWindow.init(config_file);

			// // These two guys are shared between CAEN and Teensy
			// i_run_dir 		= config_file["File"]["RunDir"].value_or("./RUNS");
			// i_run_name 		= config_file["File"]["RunName"].value_or("Testing");

			// // Teensy initial state
			// tgui_state.CurrentState = TeensyControllerStates::Standby;
			// tgui_state.Port 		= t_conf["Port"].value_or("COM4");
			// tgui_state.RunDir 		= i_run_dir;
			// tgui_state.RunName 		= i_run_name;

			// tgui_state.RTDOnlyMode = t_conf["RTDMode"].value_or(false);
			// tgui_state.RTDSamplingPeriod = t_conf["RTDSamplingPeriod"].value_or(100Lu);
			// tgui_state.RTDMask = t_conf["RTDMask"].value_or(0xFFFFLu);

			// // Send initial states of the PIDs
			// tgui_state.PeltierState = false;

			// tgui_state.PIDTempTripPoint = t_conf["PeltierTempTripPoint"].value_or(0.0f);
			// tgui_state.PIDTempValues.SetPoint = t_conf["PeltierTempSetpoint"].value_or(0.0f);
			// tgui_state.PIDTempValues.Kp = t_conf["PeltierTKp"].value_or(0.0f);
			// tgui_state.PIDTempValues.Ti = t_conf["PeltierTTi"].value_or(0.0f);
			// tgui_state.PIDTempValues.Td = t_conf["PeltierTTd"].value_or(0.0f);

			// // CAEN initial state
			// cgui_state.CurrentState = CAENInterfaceStates::Standby;
			// cgui_state.RunDir = i_run_dir;
			// cgui_state.RunName = i_run_name;
			// cgui_state.SiPMParameters = file_conf["SiPMParameters"].value_or("default");

			// cgui_state.Model = CAENDigitizerModels_map.at(CAEN_conf["Model"].value_or("DT5730B"));
			// cgui_state.PortNum = CAEN_conf["Port"].value_or(0u);

			// cgui_state.GlobalConfig.MaxEventsPerRead
			// 	= CAEN_conf["MaxEventsPerRead"].value_or(512Lu);
			// cgui_state.GlobalConfig.RecordLength
			// 	= CAEN_conf["RecordLength"].value_or(2048Lu);
			// cgui_state.GlobalConfig.PostTriggerPorcentage
			// 	= CAEN_conf["PostBufferPorcentage"].value_or(50u);
			// cgui_state.GlobalConfig.TriggerOverlappingEn
			// 	= CAEN_conf["OverlappingRejection"].value_or(false);
			// cgui_state.GlobalConfig.EXTasGate
			// 	= CAEN_conf["TRGINasGate"].value_or(false);
			// cgui_state.GlobalConfig.EXTTriggerMode
			// 	= static_cast<CAEN_DGTZ_TriggerMode_t>(CAEN_conf["ExternalTrigger"].value_or(0L));
			// cgui_state.GlobalConfig.SWTriggerMode
			// 	= static_cast<CAEN_DGTZ_TriggerMode_t>(CAEN_conf["SoftwareTrigger"].value_or(0L));
			// cgui_state.GlobalConfig.TriggerPolarity
			// 	= static_cast<CAEN_DGTZ_TriggerPolarity_t>(CAEN_conf["Polarity"].value_or(0L));
			// cgui_state.GlobalConfig.IOLevel
			// 	= static_cast<CAEN_DGTZ_IOLevel_t>(CAEN_conf["IOLevel"].value_or(0));
			// // We check how many CAEN.groupX there are and create that many
			// // groups.
			// const uint8_t MAX_CHANNELS = 64;
			// for(uint8_t ch = 0; ch < MAX_CHANNELS; ch++) {
			// 	std::string ch_toml = "group" + std::to_string(ch);
			// 	if(auto ch_conf = CAEN_conf[ch_toml].as_table()) {

			// 		cgui_state.GroupConfigs.emplace_back(
			// 			CAENGroupConfig{
			// 				.Number = ch,
			// 				.TriggerMask = CAEN_conf[ch_toml]["TrgMask"].value_or<uint8_t>(0),
			// 				.AcquisitionMask = CAEN_conf[ch_toml]["AcqMask"].value_or<uint8_t>(0),
			// 				.DCOffset = CAEN_conf[ch_toml]["Offset"].value_or<uint16_t>(0x8000u),
			// 				.DCRange = CAEN_conf[ch_toml]["Range"].value_or<uint8_t>(0u),
			// 				.TriggerThreshold = CAEN_conf[ch_toml]["Threshold"].value_or<uint16_t>(0x8000u)
			// 			}
			// 		);

			// 		if(toml::array* arr = CAEN_conf[ch_toml]["Corrections"].as_array()) {
			// 			spdlog::info("Corrections exist");
			// 			size_t j = 0;
			// 			CAENGroupConfig& last_config = cgui_state.GroupConfigs.back();
			// 			for(toml::node& elem : *arr) {
			// 				// Max number of channels there can be is 8
			// 				if(j < 8){
			// 					last_config.DCCorrections.emplace_back(
			// 						elem.value_or(0u)
			// 					);
			// 					j++;
			// 				}
			// 			}
			// 		}

			// 	}
			// }
		}

		// No copying
		GUIManager(const GUIManager&) = delete;

		~GUIManager() { }

		void operator()() {

			ImPlot::StyleColorsDark();

			// ImGui::Begin("Control Window");

			// if (ImGui::BeginTabBar("ControlTabs")) {

			// 	if(ImGui::BeginTabItem("Run")) {

			// 		ImGui::InputText("Runs Directory", &i_run_dir);
			// 		if(ImGui::IsItemHovered()) {
			// 			ImGui::SetTooltip("Directory of where all the run "
			// 				"files are going to be saved at.\n"
			// 				"Fixed when the connect buttons are pressed.");
			// 		}

			// 		ImGui::InputText("Run name", &i_run_name);
			// 		if(ImGui::IsItemHovered()) {
			// 			ImGui::SetTooltip("Name of the run.\n"
			// 				"Fixed when the connect buttons are pressed.");
			// 		}

			// 		ImGui::InputText("SiPM run name", &cgui_state.SiPMParameters);
			// 		if(ImGui::IsItemHovered()) {
			// 			ImGui::SetTooltip("This will appended to the name "
			// 				"of the SiPM pulse file to denote that the these "
			// 				"SiPM pulses where taken during this run but "
			// 				"with different parameters (OV for example).\n"
			// 				"Fixed when the Start SiPM data taking buttons "
			// 				"is pressed.");
			// 		}

			// 		// This is the only button that does send a task to all
			// 		// (so far) threads.
			// 		// TODO(Hector): generalize this in the future
			// 		CAENControlFac.Button(
			// 			"Start Data Taking",
			// 			[=](CAENInterfaceData& state) {
			// 			// Only change state if its in a work related
			// 			// state, i.e oscilloscope mode
			// 			if(state.CurrentState == CAENInterfaceStates::OscilloscopeMode ||
			// 				state.CurrentState == CAENInterfaceStates::StatisticsMode) {
			// 				state.CurrentState = CAENInterfaceStates::RunMode;
			// 				state.SiPMParameters = cgui_state.SiPMParameters;
			// 			}
			// 			return true;
			// 		});

			// 		CAENControlFac.Button(
			// 			"Stop Data Taking",
			// 			[=](CAENInterfaceData& state) {
			// 			if(state.CurrentState == CAENInterfaceStates::RunMode) {
			// 				state.CurrentState = CAENInterfaceStates::OscilloscopeMode;
			// 				state.SiPMParameters = cgui_state.SiPMParameters;
			// 			}
			// 			return true;
			// 		});
			// 			// _teensyQueueF([=](TeensyControllerState& oldState) {
			// 			// 	// There is nothing to do here technically
			// 			// 	// but I am including it just in case
			// 			// 	return true;
			// 			// });
			// 			// _caenQueueF();


			// 		ImGui::EndTabItem();
			// 	}

			// 	teensy_tabs();
			// 	caen_tabs();
			// 	ImGui::EndTabBar();
			// }
			// ImGui::End();
			controlWindow();

			//// Plots
			ImGui::Begin("Teensy-BME280 Plots"); 

			// This functor updates the plots values from the queue.
			_indicatorReceiver();

			const auto g_axis_flags = ImPlotAxisFlags_AutoFit;
			// /// Teensy-BME280 Plots
			if(ImGui::Button("Clear")) {
				_indicatorReceiver.clear_plot(IndicatorNames::LOCAL_BME_Humidity);
				_indicatorReceiver.clear_plot(IndicatorNames::LOCAL_BME_Temps);
				_indicatorReceiver.clear_plot(IndicatorNames::LOCAL_BME_Pressure);
			}
			if (ImGui::BeginTabBar("Other Plots")) {
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

				if (ImGui::BeginTabItem("Pressures")) {
					if(ImPlot::BeginPlot("Pressures", ImVec2(-1, 0))) {
						ImPlot::SetupAxes("time [s]", "P [Bar]", g_axis_flags, g_axis_flags);
						_indicatorReceiver.plot(IndicatorNames::VACUUM_PRESS, "Vacuum line");

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
				_indicatorReceiver.clear_plot(IndicatorNames::RTD_TEMP_ONE);
				_indicatorReceiver.clear_plot(IndicatorNames::RTD_TEMP_TWO);
				_indicatorReceiver.clear_plot(IndicatorNames::RTD_TEMP_THREE);
				_indicatorReceiver.clear_plot(IndicatorNames::PELTIER_CURR);

				_indicatorReceiver.clear_plot(IndicatorNames::VACUUM_PRESS);
			}

			if (ImPlot::BeginPlot("PIDs", ImVec2(-1,0))) {
				ImPlot::SetupAxes("time [s]", "Temperature [degC]", g_axis_flags, g_axis_flags);
				ImPlot::SetupAxis(ImAxis_Y2, "Current [A]", g_axis_flags | ImPlotAxisFlags_Opposite);

				_indicatorReceiver.plot(IndicatorNames::RTD_TEMP_ONE, "RTD1");

				ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
				_indicatorReceiver.plot(IndicatorNames::PELTIER_CURR, "Peltier");

				ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
				_indicatorReceiver.plot(IndicatorNames::RTD_TEMP_TWO, "RTD2");

				ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
				_indicatorReceiver.plot(IndicatorNames::RTD_TEMP_THREE, "RTD3");

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

			indicatorWindow();
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
				[](CAENInterfaceData& state) {
					state.CurrentState = CAENInterfaceStates::Closing;
					return true;
				}
			);
		}

	};

} // namespace SBCQueens
