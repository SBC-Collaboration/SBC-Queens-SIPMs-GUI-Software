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
			controlWindow		(TeensyControlFac, tgui_state, CAENControlFac,
								cgui_state, other_state) {

		}

		// No copying
		GUIManager(const GUIManager&) = delete;

		~GUIManager() { }

		void operator()() {

			ImPlot::StyleColorsDark();

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
