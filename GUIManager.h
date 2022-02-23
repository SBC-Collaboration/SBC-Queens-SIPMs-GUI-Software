#pragma once

#include <cstdint>
#include <math.h>
#include <string>
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
#include "deps/imgui/imgui.h"
#include "imgui.h"
#include "imgui_helpers.h"
#include "implot_helpers.h"
#include "indicators.h"

#define MAX_CHANNELS 4

namespace SBCQueens {

	template<typename... QueueFuncs>
	class GUIManager
	{
private:

		std::tuple<QueueFuncs&...> _queues;
		IndicatorReceiver<IndicatorNames> _plotManager;

		std::function<bool(TeensyQueueInType&&)> _teensyQueueF;
		std::function<bool(CAENQueueType&&)> _caenQueueF;

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
		InputFloat pid_one_PIDTTD_inpF;

		InputFloat pid_one_PIDAKP_inpF;
		InputFloat pid_one_PIDATI_inpF;
		InputFloat pid_one_PIDATD_inpF;

		// PID2 Controls
		Checkbox pid_two_chkBox;
		InputFloat pid_two_temp_sp_inpF;
		InputFloat pid_two_curr_sp_inpF;
		InputFloat pid_two_PIDTKP_inpF;
		InputFloat pid_two_PIDTTI_inpF;
		InputFloat pid_two_PIDTTD_inpF;

		InputFloat pid_two_PIDAKP_inpF;
		InputFloat pid_two_PIDATI_inpF;
		InputFloat pid_two_PIDATD_inpF;

		// CAEN Controls
		Button caen_connect_btn;
		Button caen_disconnect_btn;
		Button send_soft_trig;
		Button start_processing_btn;

		std::string ic_conf_model;
		int ic_maxEventsPerRead;
		int ic_overlapping_rej;
		int ic_recordLength;
		int ic_postbuffer;
		bool ic_ext_as_gate;
		int ic_ext_trigger;
		int ic_soft_trigger;
		int	ic_ch_polaritiy;

		std::array<int, MAX_CHANNELS> 	ic_ch_number;
		std::array<bool, MAX_CHANNELS> 	ic_ch_enabled;
		std::array<int, MAX_CHANNELS>   ic_ch_trgmask;
		std::array<int, MAX_CHANNELS>   ic_ch_acqmask;
		std::array<int, MAX_CHANNELS> 	ic_ch_offsets;
		std::array<std::array<int, 8>, MAX_CHANNELS> 	ic_ch_corrections;
		std::array<int, MAX_CHANNELS> 	ic_ch_thresholds;
		std::array<int, MAX_CHANNELS> 	ic_ch_ranges;


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

		SiPMPlot sipm_plot_zero;
		SiPMPlot sipm_plot_one;
		SiPMPlot sipm_plot_two;
		SiPMPlot sipm_plot_three;

		std::string i_com_port;
		std::string i_run_dir;
		std::string i_run_name;
		std::string i_sipmRunName;

public:
		explicit GUIManager(QueueFuncs&... queues) : 
			_queues(forward_as_tuple(queues...)),
			_plotManager(std::get<SiPMsPlotQueue&>(_queues)) {

			TeensyInQueue& teensyQueue = std::get<TeensyInQueue&>(_queues);
			CAENQueue& caenQueue = std::get<CAENQueue&>(_queues);

			_teensyQueueF = [&](TeensyQueueInType&& f) -> bool {
				return teensyQueue.try_enqueue(f);
			};

			_caenQueueF = [&](CAENQueueType&& f) -> bool {
				return caenQueue.try_enqueue(f);
			};

			// Controls
			auto config = toml::parse_file("gui_setup.toml");
			auto t_conf = config["Teensy"];
			auto CAEN_conf = config["CAEN"];

			connect_btn = make_button("Connect");
			disconnect_btn = make_button("Disconnect");

			pid_relay_chkBox = make_checkbox("PID Relay State",
				t_conf["PIDRelay"].value_or(false));
			gen_relay_chkBox = make_checkbox("General Relay State",
				t_conf["GeneralRelay"].value_or(false));

			// PID1
			pid_one_chkBox = make_checkbox("PID1 Enable",
				t_conf["PID1Enable"].value_or(false));

			pid_one_temp_sp_inpF = make_input_float("Temp Setpoint 1",
				t_conf["PID1TempSetpoint"].value_or(0.0f));
			pid_one_curr_sp_inpF = make_input_float("Current Setpoint 1",
				t_conf["PID1CurrentSetpoint"].value_or(3.0f));

			pid_one_PIDTKP_inpF = make_input_float("Tkp 1",
				t_conf["PID1TKp"].value_or(1000.0f));
			pid_one_PIDTTI_inpF = make_input_float("Tki 1",
				t_conf["PID1TTi"].value_or(100.0f));
			pid_one_PIDTTD_inpF = make_input_float("Tkd 1",
				t_conf["PID1TTd"].value_or(0.0f));

			pid_one_PIDAKP_inpF = make_input_float("Akp 1",
				t_conf["PID1AKp"].value_or(200.0f));
			pid_one_PIDATI_inpF = make_input_float("Aki 1",
				t_conf["PID1ATi"].value_or(100.0f));
			pid_one_PIDATD_inpF = make_input_float("Akd 1",
				t_conf["PID1ATd"].value_or(0.0f));

			pid_two_chkBox = make_checkbox("PID2 Enable",
				t_conf["PID2Enable"].value_or(false));
			pid_two_temp_sp_inpF = make_input_float("Temp Setpoint 2",
				t_conf["PID2TempSetpoint"].value_or(0.0f));
			pid_two_curr_sp_inpF = make_input_float("Current Setpoint 2",
				t_conf["PID2CurrentSetpoint"].value_or(3.0f));

			pid_two_PIDTKP_inpF = make_input_float("Tkp 2",
				t_conf["PID2TKp"].value_or(1000.0f));
			pid_two_PIDTTI_inpF = make_input_float("Tki 2",
				t_conf["PID2TTi"].value_or(100.0f));
			pid_two_PIDTTD_inpF = make_input_float("Tkd 2",
				t_conf["PID2TTd"].value_or(0.0f));

			pid_two_PIDAKP_inpF = make_input_float("Akp 2",
				t_conf["PID2AKp"].value_or(200.0f));
			pid_two_PIDATI_inpF = make_input_float("Aki 2",
				t_conf["PID2ATi"].value_or(100.0f));
			pid_two_PIDATD_inpF = make_input_float("Akd 2",
				t_conf["PID2ATd"].value_or(0.0f));
			// End Teensy Controls

			// CAEN Controls
			caen_connect_btn 		= make_button("Connect");
			caen_disconnect_btn 	= make_button("Disconnect");
			send_soft_trig 			= make_button("Software Trigger");
			start_processing_btn 	= make_button("Start Processing");

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
			frequency = make_indicator(IndicatorNames::FREQUENCY, 4,
				NumericFormat::Scientific);
			_plotManager.add(frequency);
			dark_noise = make_indicator(IndicatorNames::DARK_NOISE_RATE, 3,
				NumericFormat::Scientific);
			_plotManager.add(dark_noise);
			gain = make_indicator(IndicatorNames::GAIN, 3,
				NumericFormat::Scientific);
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

			sipm_plot_zero = make_plot(IndicatorNames::SiPM_Plot_ZERO);
			sipm_plot_zero.ClearOnNewData = true;
			_plotManager.add(sipm_plot_zero);

			sipm_plot_one = make_plot(IndicatorNames::SiPM_Plot_ONE);
			sipm_plot_one.ClearOnNewData = true;
			_plotManager.add(sipm_plot_one);

			sipm_plot_two = make_plot(IndicatorNames::SiPM_Plot_TWO);
			sipm_plot_two.ClearOnNewData = true;
			_plotManager.add(sipm_plot_two);

			sipm_plot_three = make_plot(IndicatorNames::SiPM_Plot_THREE);
			sipm_plot_three.ClearOnNewData = true;
			_plotManager.add(sipm_plot_three);

			i_com_port 		= t_conf["Port"].value_or("COM4");
			i_run_dir 		= config["File"]["RunDir"].value_or("./RUNS");
			i_run_name 		= config["File"]["RunName"].value_or("Testing");
			i_sipmRunName 	= config["File"]["SiPMParameters"].value_or("default");

			ic_conf_model 		= CAEN_conf["Model"].value_or("DT5730B");
			ic_maxEventsPerRead = CAEN_conf["MaxEventsPerRead"].value_or(512u);
			ic_recordLength 	= CAEN_conf["RecordLength"].value_or(2048u);
			ic_postbuffer 		= CAEN_conf["PostBufferPorcentage"].value_or(50u);
			ic_overlapping_rej  = CAEN_conf["OverlappingRejection"].value_or(0u);
			ic_ext_as_gate 		= CAEN_conf["TRGINasGate"].value_or(false);
			ic_ext_trigger 		= CAEN_conf["ExternalTrigger"].value_or(0);
			ic_soft_trigger 	= CAEN_conf["SoftwareTrigger"].value_or(0);
			ic_ch_polaritiy 	= CAEN_conf["Polarity"].value_or(0u);

			for(auto& group : ic_ch_corrections) {
				group.fill(0);
			}

			for(int i = 0; i < MAX_CHANNELS; i++) {
				std::string ch_toml = "group" + std::to_string(i);
				ic_ch_number[i] 	= CAEN_conf[ch_toml]["Number"].value_or(0u);
				ic_ch_enabled[i] 	= CAEN_conf[ch_toml]["Enabled"].value_or(false);
				ic_ch_trgmask[i] 	= CAEN_conf[ch_toml]["TrgMask"].value_or(0u);
				ic_ch_acqmask[i] 	= CAEN_conf[ch_toml]["AcqMask"].value_or(0u);
				ic_ch_offsets[i] 	= CAEN_conf[ch_toml]["Offset"].value_or(0x8000u);
				if(toml::array* arr = CAEN_conf[ch_toml]["Corrections"].as_array()) {
					spdlog::info("Corrections exist");
					size_t j = 0;
					for(toml::node& elem : *arr) {
						if(j < 8){
							ic_ch_corrections[i][j] = elem.value_or(0u);
							j++;
						}
					}
				}

				ic_ch_thresholds[i] = CAEN_conf[ch_toml]["Threshold"].value_or(0x8000u);
				ic_ch_ranges[i]		= CAEN_conf[ch_toml]["Range"].value_or(0u);
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

					ImGui::InputText("SiPM run name", &i_sipmRunName);
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
					if(ImGui::Button("Start SiPM data taking")) {
						_teensyQueueF([=](TeensyControllerState& oldState) {
							// There is nothing to do here technically
							// but I am including it just in case
							return true;
						});
						_caenQueueF([=](CAENInterfaceState& state) {
							// Only change state if its in a work related
							// state, i.e oscilloscope mode
							if(state.CurrentState == CAENInterfaceStates::OscilloscopeMode ||
								state.CurrentState == CAENInterfaceStates::StatisticsMode) {
								state.CurrentState = CAENInterfaceStates::RunMode;
								state.SiPMParameters = i_sipmRunName;
							}
							return true;
						});
					}

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


			ImGui::Begin("SiPM Plot");  
			if (ImPlot::BeginPlot("SiPM Trace", ImVec2(-1,0))) {

				ImPlot::SetupAxes("time [ns]", "Counts", g_axis_flags, g_axis_flags);

				sipm_plot_zero("Plot 1");
				sipm_plot_one("Plot 2");
				sipm_plot_two("Plot 3");
				sipm_plot_three("Plot 4");

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
		 	_teensyQueueF(
		 		[](TeensyControllerState& oldState) {
					oldState.CurrentState = TeensyControllerStates::Closing;
					return true;
				}
			);

			_caenQueueF(
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
				static int model = 0;
				try {
					model = static_cast<int>(
						CAENDigitizerModels_map.at(ic_conf_model)
					);
				} catch (...) {
					model = static_cast<int>(
						CAENDigitizerModels_map.at("DT5730B")
					);
				}

				ImGui::Combo("Model", &model, "DT5730B\0DT5740D\0\0");

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
				if(caen_connect_btn(_caenQueueF,
					[=](CAENInterfaceState& state) {
						state.Model =
							static_cast<CAENDigitizerModel>(model);
						state.PortNum = caen_port;

						state.GlobalConfig.MaxEventsPerRead =
							static_cast<uint32_t>(ic_maxEventsPerRead);
						state.GlobalConfig.RecordLength =
							static_cast<uint32_t>(ic_recordLength);
						state.GlobalConfig.PostTriggerPorcentage =
							static_cast<uint32_t>(ic_postbuffer);
						state.GlobalConfig.TriggerOverlappingEn =
							ic_overlapping_rej > 0;

						state.GlobalConfig.EXTasGate = ic_ext_as_gate;
						state.GlobalConfig.EXTTriggerMode =
							static_cast<CAEN_DGTZ_TriggerMode_t>(ic_ext_trigger);

						state.GlobalConfig.TriggerPolarity
							= static_cast<CAEN_DGTZ_TriggerPolarity_t>(ic_ch_polaritiy);

						// For now only 4 channels are allowed
						std::vector<CAENGroupConfig> new_ch_configs;

						for(int i= 0; i < MAX_CHANNELS; i++) {

							// If channel is disabled, do not add to the list
							if(!ic_ch_enabled[i]) {
								continue;
							}

							new_ch_configs.emplace_back(CAENGroupConfig {

								.Number
									= static_cast<uint8_t>(ic_ch_number[i]),
								.TriggerMask
									= static_cast<uint8_t>(ic_ch_trgmask[i]),
								.AcquisitionMask
									= static_cast<uint8_t>(ic_ch_acqmask[i]),
								.DCOffset
									= static_cast<uint32_t>(ic_ch_offsets[i]),
								.DCCorrections
									= std::vector<uint8_t>(
										ic_ch_corrections[i].cbegin(),
										ic_ch_corrections[i].cend()),
								.DCRange
									= static_cast<uint8_t>(ic_ch_ranges[i]),
								.TriggerThreshold
									= static_cast<uint32_t>(ic_ch_thresholds[i]),

							});
						}

						state.ChannelConfigs = new_ch_configs;

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

				if(caen_disconnect_btn(_caenQueueF,
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

				ImGui::InputInt("Max Events Per Read", &ic_maxEventsPerRead);

				ImGui::InputInt("Record Length [counts]", &ic_recordLength);
				ImGui::InputInt("Post-Trigger buffer %", &ic_postbuffer);

    			ImGui::Text("Overlapping Rejection:"); ImGui::SameLine();
    			ImGui::RadioButton("Allowed",
    				&ic_overlapping_rej, 1); ImGui::SameLine();
    			ImGui::RadioButton("Not Allowed",
    				&ic_overlapping_rej, 0);

    			ImGui::Checkbox("TRG-IN as Gate", &ic_ext_as_gate);

    			ImGui::Combo("External Trigger Mode", &ic_ext_trigger,
    				"Disabled\0Acq Only\0Ext Only\0Both\0\0");
    			ImGui::Combo("Software Trigger Mode", &ic_soft_trigger,
    				"Disabled\0Acq Only\0Ext Only\0Both\0\0");

    			ImGui::Text("Trigger Polarity:"); ImGui::SameLine();
	    			ImGui::RadioButton("Positive", &ic_ch_polaritiy, 0); ImGui::SameLine();
	    			ImGui::RadioButton("Negative", &ic_ch_polaritiy, 1);

				send_soft_trig(_caenQueueF,
					[](CAENInterfaceState& state) {
						software_trigger(state.Port);
						return true;
					}
				);
				if(ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Forces a trigger in the digitizer if "
						"the feature is enabled");
				}

    			start_processing_btn(_caenQueueF,
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
			for(int i = 0; i < MAX_CHANNELS; i++) {
				std::string tab_name = "CAEN GR" + std::to_string(i);
				if(ImGui::BeginTabItem(tab_name.c_str())) {
					ImGui::PushItemWidth(120);
					ImGui::Checkbox("Enabled", &ic_ch_enabled[i]);
					ImGui::InputInt("Group #", &ic_ch_number[i]);
					ImGui::InputInt("DC Offset [counts]", &ic_ch_offsets[i]);
					ImGui::InputInt("Threshold [counts]", &ic_ch_thresholds[i]);
					ImGui::InputInt("Trigger Mask", &ic_ch_trgmask[i]);
					ImGui::InputInt("Acquisition Mask", &ic_ch_acqmask[i]);

					for(int j = 0; j < 8; j++) {
						std::string c_name = "Correction " + std::to_string(j);
						ImGui::InputInt(c_name.c_str(), &ic_ch_corrections[i][j]);
					}

					// TODO(Hector): change this to get the actual
					// ranges of the digitizer used, maybe change it to a dropbox?
	    			ImGui::RadioButton("2V", &ic_ch_ranges[i], 0); ImGui::SameLine();
	    			ImGui::RadioButton("0.5V", &ic_ch_ranges[i], 1);

	    			ImGui::EndTabItem();
				}

			}

		}

		void teensy_tabs() {
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
				ImGui::InputText("Teensy COM port", &i_com_port);

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
				if(connect_btn(_teensyQueueF,
					// To make things secure, we pass everything by value
					[=](TeensyControllerState& oldState) {
						// We capture the com_value with connect on top of
						// the button state which in this case is not
						// going to be used.
						oldState.Port = i_com_port;
						oldState.RunDir = i_run_dir;
						oldState.RunName = i_run_name;

						// Send initial states of the PIDs
						oldState.PIDOneTempValues.SetPoint
							= pid_one_temp_sp_inpF.Get();
						oldState.PIDOneTempValues.Kp
							= pid_one_PIDTKP_inpF.Get();
						oldState.PIDOneTempValues.Ti
							= pid_one_PIDTTI_inpF.Get();
						oldState.PIDOneTempValues.Td
							= pid_one_PIDTTD_inpF.Get();

						oldState.PIDOneCurrentValues.SetPoint
							= pid_one_curr_sp_inpF.Get();
						oldState.PIDOneCurrentValues.Kp
							= pid_one_PIDAKP_inpF.Get();
						oldState.PIDOneCurrentValues.Ti
							= pid_one_PIDATI_inpF.Get();
						oldState.PIDOneCurrentValues.Td
							= pid_one_PIDATD_inpF.Get();

						oldState.PIDTwoTempValues.SetPoint
							= pid_two_temp_sp_inpF.Get();
						oldState.PIDTwoTempValues.Kp
							= pid_two_PIDTKP_inpF.Get();
						oldState.PIDTwoTempValues.Ti
							= pid_two_PIDTTI_inpF.Get();
						oldState.PIDTwoTempValues.Td
							= pid_two_PIDTTD_inpF.Get();

						oldState.PIDTwoCurrentValues.SetPoint
							= pid_two_curr_sp_inpF.Get();
						oldState.PIDTwoCurrentValues.Kp
							= pid_two_PIDAKP_inpF.Get();
						oldState.PIDTwoCurrentValues.Ti
							= pid_two_PIDATI_inpF.Get();
						oldState.PIDTwoCurrentValues.Td
							= pid_two_PIDATD_inpF.Get();

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

				if(disconnect_btn(_teensyQueueF,
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

				pid_relay_chkBox(_teensyQueueF,
					ImGui::IsItemEdited,
					[=](TeensyControllerState& oldState) {
						oldState.PIDRelayState = pid_relay_chkBox.Get();
						return send_teensy_cmd(TeensyCommands::SetPIDRelay, oldState);
					}
				);

				gen_relay_chkBox(_teensyQueueF,
					ImGui::IsItemEdited,
					[=](TeensyControllerState& oldState) {
						oldState.GeneralRelayState = gen_relay_chkBox.Get();
						return send_teensy_cmd(TeensyCommands::SetGeneralRelay, oldState);
					}
				);

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("PID1")) {
				ImGui::PushItemWidth(180);
				pid_one_chkBox(_teensyQueueF,
					ImGui::IsItemEdited,
					[=](TeensyControllerState& oldState) {
						bool state = pid_one_chkBox.Get();
						oldState.PIDOneState = state ? SBCQueens::PIDState::Running : SBCQueens::PIDState::Standby;
						return state ? send_teensy_cmd(TeensyCommands::StartPIDOne, oldState) : send_teensy_cmd(TeensyCommands::StopPIDOne, oldState);
					}
				);

				ImGui::Text("PID Setpoints");
				pid_one_temp_sp_inpF(_teensyQueueF,
					0.01f, 6.0f, "%.6f °C", ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.PIDOneTempValues.SetPoint = pid_one_temp_sp_inpF.Get();
						return send_teensy_cmd(TeensyCommands::SetPIDOneTempSetpoint, oldState);
					}
				);

				pid_one_curr_sp_inpF(_teensyQueueF,
					0.01f, 6.0f, "%.6f A", ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.PIDOneCurrentValues.SetPoint = pid_one_curr_sp_inpF.Get();
						return send_teensy_cmd(TeensyCommands::SetPIDOneCurrSetpoint, oldState);
					}
				);

				ImGui::Text("Temperature PID coefficients.");
				pid_one_PIDTKP_inpF(_teensyQueueF,
					0.01f, 6.0f, "%.6f", ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.PIDOneTempValues.Kp = pid_one_PIDTKP_inpF.Get();
						return send_teensy_cmd(TeensyCommands::SetPIDOneTempKp, oldState);
					}
				);
				pid_one_PIDTTI_inpF(_teensyQueueF,
					0.01f, 6.0f, "%.6f ms", ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.PIDOneTempValues.Ti = pid_one_PIDTTI_inpF.Get();
						return send_teensy_cmd(TeensyCommands::SetPIDOneTempTi, oldState);
					}
				);
				pid_one_PIDTTD_inpF(_teensyQueueF,
					0.01f, 6.0f, "%.6f ms", ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.PIDOneTempValues.Td = pid_one_PIDTTD_inpF.Get();
						return send_teensy_cmd(TeensyCommands::SetPIDOneTempTd, oldState);
					}
				);

				ImGui::Text("Current PID coefficients.");
				pid_one_PIDAKP_inpF(_teensyQueueF,
					0.01f, 6.0f, "%.6f", ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.PIDOneCurrentValues.Kp = pid_one_PIDAKP_inpF.Get();
						return send_teensy_cmd(TeensyCommands::SetPIDOneCurrKp, oldState);
					}
				);
				pid_one_PIDATI_inpF(_teensyQueueF,
					0.01f, 6.0f, "%.6f ms", ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.PIDOneCurrentValues.Ti = pid_one_PIDATI_inpF.Get();
						return send_teensy_cmd(TeensyCommands::SetPIDOneCurrTi, oldState);
					}
				);
				pid_one_PIDATD_inpF(_teensyQueueF,
					0.01f, 6.0f, "%.6f ms", ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.PIDOneCurrentValues.Td = pid_one_PIDATD_inpF.Get();
						return send_teensy_cmd(TeensyCommands::SetPIDOneCurrTd, oldState);
					}
				);

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("PID2")) {
				ImGui::PushItemWidth(180);
				pid_two_chkBox(_teensyQueueF,
					ImGui::IsItemEdited,
					[=](TeensyControllerState& oldState) {
						bool state = pid_two_chkBox.Get();
						oldState.PIDTwoState = state ? SBCQueens::PIDState::Running : SBCQueens::PIDState::Standby;
						return state ? send_teensy_cmd(TeensyCommands::StartPIDTwo, oldState) : send_teensy_cmd(TeensyCommands::StopPIDTwo, oldState);
					}
				);

				ImGui::Text("PID Setpoints");
				pid_two_temp_sp_inpF(_teensyQueueF,
					0.01f, 6.0f, "%.6f °C", ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.PIDTwoTempValues.SetPoint = pid_two_temp_sp_inpF.Get();
						return send_teensy_cmd(TeensyCommands::SetPIDTwoTempSetpoint, oldState);
					}
				);
				pid_two_curr_sp_inpF(_teensyQueueF,
					0.01f, 6.0f, "%.6f A", ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.PIDTwoCurrentValues.SetPoint = pid_two_curr_sp_inpF.Get();
						return send_teensy_cmd(TeensyCommands::SetPIDTwoCurrSetpoint, oldState);
					}
				);

				ImGui::Text("Temperature PID coefficients.");
				pid_two_PIDTKP_inpF(_teensyQueueF,
					0.01f, 6.0f, "%.6f", ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.PIDTwoTempValues.Kp = pid_two_PIDTKP_inpF.Get();
						return send_teensy_cmd(TeensyCommands::SetPIDTwoTempKp, oldState);
					}
				);
				pid_two_PIDTTI_inpF(_teensyQueueF,
					0.01f, 6.0f, "%.6f ms", ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.PIDTwoTempValues.Ti = pid_two_PIDTTI_inpF.Get();
						return send_teensy_cmd(TeensyCommands::SetPIDTwoTempTi, oldState);
					}
				);
				pid_two_PIDTTD_inpF(_teensyQueueF,
					0.01f, 6.0f, "%.6f ms", ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.PIDTwoTempValues.Td = pid_two_PIDTTD_inpF.Get();
						return send_teensy_cmd(TeensyCommands::SetPIDTwoTempTd, oldState);
					}
				);

				ImGui::Text("Current PID coefficients.");
				pid_two_PIDAKP_inpF(_teensyQueueF,
					0.01f, 6.0f, "%.6f", ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.PIDTwoCurrentValues.Kp = pid_two_PIDAKP_inpF.Get();
						return send_teensy_cmd(TeensyCommands::SetPIDTwoCurrKp, oldState);
					}
				);
				pid_two_PIDATI_inpF(_teensyQueueF,
					0.01f, 6.0f, "%.6f ms", ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.PIDTwoCurrentValues.Ti = pid_two_PIDATI_inpF.Get();
						return send_teensy_cmd(TeensyCommands::SetPIDTwoCurrTi, oldState);
					}
				);
				pid_two_PIDATD_inpF(_teensyQueueF,
					0.01f, 6.0f, "%.6f ms", ImGui::IsItemDeactivated,
					[=](TeensyControllerState& oldState) {
						oldState.PIDTwoCurrentValues.Td = pid_two_PIDATD_inpF.Get();
						return send_teensy_cmd(TeensyCommands::SetPIDTwoCurrTd, oldState);
					}
				);


				ImGui::EndTabItem();
			}
		}

	};

} // namespace SBCQueens