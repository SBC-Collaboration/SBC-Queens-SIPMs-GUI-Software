#include "GUIManager.h"

namespace SBCQueens {

	// template<typename SenderFunc>
	// void GUIManager<SenderFunc>::operator()() {

	// 	static char com_port[16] = "COM8";

	// 	static bool pid1_enabled = false;

	// 	static bool pid2_enabled = false;
	// 	static float pid2_temp_setpoint = 25.0; // C
	// 	static float pid2_curr_setpoint = 1.0; // Amps
	// 	static float pid2_temp_kp = 0.0, pid2_temp_ti = 0.0, pid2_temp_td = 0.0;
	// 	static float pid2_curr_kp = 0.0, pid2_curr_ti = 0.0, pid2_curr_td = 0.0;

	// 	ImGui::Begin("Control Window");  

	// 	if (ImGui::BeginTabBar("ControlTabs")) {
	// 		if (ImGui::BeginTabItem("Teensy-Controller")) {

	// 			// This makes all the items between PushItemWidth & PopItemWidth 80 px (?) wide
	// 			ImGui::PushItemWidth(80);

	// 			// 1.0 -> 0.5
	// 			// 0.5 -> 1.0
	// 			// (1.0 - 0.5) / (0.5 - 1.0) = -1
	// 			static float connected_mod = 1.0;
	// 			static bool local_is_connected = false;

	// 			// Com port text input
	// 			SBCQueens::InputText("Teensy COM port", com_port, ImGui::IsItemEdited, _queueFunc);
	// 			// ImGui::InputText("Teensy COM port", com_port, IM_ARRAYSIZE(com_port));
	// 			// if(ImGui::IsItemEdited()) {
	// 			// 	g_com_port_mutex.lock();
	// 			// 	g_com_port = com_port;
	// 			// 	g_com_port_mutex.unlock();
	// 			// }

	// 			// This button starts the teensy communication
	// 			ImGui::SameLine();
	// 			// Colors to pop up or shadow it depending on the conditions
	// 			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(2.0 / 7.0f, 0.6f, connected_mod*0.6f));
	// 			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(2.0 / 7.0f, 0.7f, connected_mod*0.7f));
	// 			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(2.0 / 7.0f, 0.8f, connected_mod*0.8f));
	// 			SBCQueens::Button("Connect", [this](auto&& ctlVal) {
	// 				local_is_connected = true;
	// 				connected_mod = 0.5;
	// 				return _queueFunc(ctlVal);
	// 			});

	// 			// if(ImGui::Button("Connect")) {
	// 			// 	local_is_connected = true;
	// 			// 	g_init_connection.store(true);
	// 			// 	connected_mod = 0.5;
	// 			// }
	// 			ImGui::PopStyleColor(3);

	// 			/// Disconnect button
	// 			float disconnected_mod = 1.5 - connected_mod;
	// 			ImGui::SameLine();
	// 			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, disconnected_mod*0.6f));
	// 			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, disconnected_mod*0.7f));
	// 			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0, 0.8f, disconnected_mod*0.8f));

	// 			SBCQueens::Button("Disconnect", [this](auto&& ctlVal) {
	// 				local_is_connected = false;
	// 				connected_mod = 1.5;
	// 				return _queueFunc(ctlVal);
	// 			});

	// 			// if(ImGui::Button("Disconnect")) {
	// 			// 	local_is_connected = false;
	// 			// 	g_init_connection.store(false);
	// 			// 	connected_mod = 1.0;
	// 			// }
	// 			ImGui::PopStyleColor(3);

	// 			ImGui::PopItemWidth();

	// 			static bool s_pid_relay_state = false;
	// 			ImGui::Checkbox("PID Relay State", &s_pid_relay_state);
	// 			if(ImGui::IsItemEdited()) {
	// 				// g_pid_relay.store(s_pid_relay_state);
	// 			}

	// 			static bool s_general_relay_state = false;
	// 			ImGui::Checkbox("General Relay State", &s_general_relay_state);
	// 			if(ImGui::IsItemEdited()) {
	// 				// g_general_relay.store(s_pid_relay_state);
	// 			}

	// 			// If the local state and the atomic do not match, it implies the connection failed
	// 			// if(g_init_connection.load() != local_is_connected) {
	// 			// 	ImGui::Text("Connection failed.");
	// 			// }

	// 			ImGui::EndTabItem();
	// 		}

	// 		if (ImGui::BeginTabItem("PID1")) {

	// 			static bool s_pid1_state = false;
	// 			SBCQueens::Checkbox("Enabled", s_pid1_state, ImGui::IsItemEdited, _queueFunc);
	// 			// ImGui::Checkbox("Enabled", &s_pid1_state);
	// 			// if(ImGui::IsItemEdited()) {
	// 			// 	// g_pid1_state.store(s_pid1_state);
	// 			// }

	// 			static double temp_float;
	// 			ImGui::Text("PID Setpoints");
	// 			SBCQueens::InputFloat("T", temp_float, 0.01f, 6.0f, "%.6f °C", ImGui::IsItemDeactivated, _queueFunc);
	// 			// ImGui::InputFloat("T", &_teensy_registers.PID1_DESIRED_TEMP_REG, 
	// 			// 	0.01f, 6.0f, "%.6f °C");
	// 			// if(ImGui::IsItemDeactivated()) {
	// 			// 	_update_pid1();
	// 			// }

	// 			SBCQueens::InputFloat("Current", temp_float, 0.01f, 6.0f, "%.6f A", ImGui::IsItemDeactivated, _queueFunc);
	// 			// ImGui::InputFloat("Current", &_teensy_registers.PID1_DESIRED_CURRENT_REG, 
	// 			// 	0.01f, 6.0f, "%.6f A");
	// 			// if(ImGui::IsItemDeactivated()) {
	// 			// 	_update_pid1();
	// 			// }

	// 			ImGui::Text("Temperature PID coefficients.");
	// 			SBCQueens::InputFloat("TKp", temp_float, 0.01f, 6.0f, "%.6f", ImGui::IsItemDeactivated, _queueFunc);
	// 			// ImGui::InputFloat("TKp", &_teensy_registers.PID1_TEMP_KP_REG, 
	// 			// 	0.01f, 6.0f, "%.6f");
	// 			// if(ImGui::IsItemDeactivated()) {
	// 			// 	_update_pid1();
	// 			// }

	// 			SBCQueens::InputFloat("TTi", temp_float, 0.01f, 6.0f, "%.6f ms", ImGui::IsItemDeactivated, _queueFunc);
	// 			// ImGui::InputFloat("TTi", &_teensy_registers.PID1_TEMP_TI_REG, 
	// 			// 	0.01f, 6.0f, "%.6f ms");
	// 			// if(ImGui::IsItemDeactivated()) {
	// 			// 	_update_pid1();
	// 			// }

	// 			SBCQueens::InputFloat("TTd", temp_float, 0.01f, 6.0f, "%.6f ms", ImGui::IsItemDeactivated, _queueFunc);
	// 			// ImGui::InputFloat("TTd", &_teensy_registers.PID1_TEMP_TD_REG, 
	// 			// 	0.01f, 6.0f, "%.6f ms");
	// 			// if(ImGui::IsItemDeactivated()) {
	// 			// 	_update_pid1();
	// 			// }

	// 			ImGui::Text("Current PID coefficients.");
	// 			SBCQueens::InputFloat("IKp", temp_float, 0.01f, 6.0f, "%.6f", ImGui::IsItemDeactivated, _queueFunc);
	// 			// ImGui::InputFloat("IKp", &_teensy_registers.PID1_CURR_KP_REG, 
	// 			// 	0.01f, 6.0f, "%.6f");
	// 			// if(ImGui::IsItemDeactivated()) {
	// 			// 	_update_pid1();
	// 			// }

	// 			SBCQueens::InputFloat("ITi", temp_float, 0.01f, 6.0f, "%.6f ms", ImGui::IsItemDeactivated, _queueFunc);
	// 			// ImGui::InputFloat("ITi", &_teensy_registers.PID1_CURR_TI_REG, 
	// 			// 	0.01f, 6.0f, "%.6f ms");
	// 			// if(ImGui::IsItemDeactivated()) {
	// 			// 	_update_pid1();
	// 			// }

	// 			SBCQueens::InputFloat("ITd", temp_float, 0.01f, 6.0f, "%.6f ms", ImGui::IsItemDeactivated, _queueFunc);
	// 			// ImGui::InputFloat("ITd", &_teensy_registers.PID1_CURR_TD_REG, 
	// 			// 	0.01f, 6.0f, "%.6f ms");
	// 			// if(ImGui::IsItemDeactivated()) {
	// 			// 	_update_pid1();
	// 			// }

	// 			ImGui::EndTabItem();
	// 		}

	// 		if (ImGui::BeginTabItem("PID2")) {

	// 			ImGui::Checkbox("Enabled", &pid2_enabled);

	// 			ImGui::Text("PID Setpoints");
	// 			ImGui::InputFloat("T", &pid2_temp_setpoint, 0.01f, 6.0f, "%.6f °C");
	// 			ImGui::InputFloat("Current", &pid2_curr_setpoint, 0.01f, 6.0f, "%.6f A");

	// 			ImGui::Text("Temperature PID coefficients.");
	// 			ImGui::InputFloat("Kp", &pid2_temp_kp, 0.01f, 6.0f, "%.6f");
	// 			ImGui::InputFloat("Ti", &pid2_temp_ti, 0.01f, 6.0f, "%.6f ms");
	// 			ImGui::InputFloat("Td", &pid2_temp_ti, 0.01f, 6.0f, "%.6f ms");

	// 			ImGui::Text("Current PID coefficients.");
	// 			ImGui::InputFloat("Kp", &pid2_curr_kp, 0.01f, 6.0f, "%.6f ms");
	// 			ImGui::InputFloat("Ti", &pid2_curr_ti, 0.01f, 6.0f, "%.6f ms");
	// 			ImGui::InputFloat("Td", &pid2_curr_td, 0.01f, 6.0f, "%.6f ms");

	// 			ImGui::EndTabItem();
	// 		}


	// 		ImGui::EndTabBar();
	// 	} 

	// 	ImGui::End();

	// 	//// Plots
	// 	ImGui::Begin("Teensy-BME280 Plots" ); 

	// 	// static ImVector<float> pid_times, bme_times;
	// 	// static ImVector<float> pid1_temps, pid1_currs, pid2_temps, pid2_currs;
	// 	// static ImVector<float> localbme_temps, localbme_press, localbme_hum;
	// 	// static ImVector<float> boxbme_temps, boxbme_press, boxbme_hum;

	// 	// if(g_new_pid_values.load()) {
	// 	// 	_teensy_values = g_teensy_values.load();

	// 	// 	pid1_temps.push_back(_teensy_values.PID1_LATEST_TEMP_REG);
	// 	// 	pid1_currs.push_back(_teensy_values.PID1_LATEST_CURRENT_REG);
	// 	// 	pid2_temps.push_back(_teensy_values.PID2_LATEST_TEMP_REG);
	// 	// 	pid2_currs.push_back(_teensy_values.PID2_LATEST_CURRENT_REG);

	// 	// 	pid_times.push_back(_teensy_values.PID_LATEST_TIME);

	// 	// 	g_new_pid_values.store(false);
	// 	// }

	// 	// if(g_new_bme280_values.load()) {
	// 	// 	_teensy_values = g_teensy_values.load();

	// 	// 	bme_times.push_back(_teensy_values.BME_LATEST_TIME);

	// 	// 	localbme_temps.push_back(_teensy_values.LOCAL_BME_LAST_TEMP_REG);
	// 	// 	localbme_press.push_back(_teensy_values.LOCAL_BME_LAST_PRESSURE_REG);
	// 	// 	localbme_hum.push_back(_teensy_values.LOCAL_BME_LAST_HUM_REG);

	// 	// 	boxbme_temps.push_back(_teensy_values.BOX_BME_LAST_TEMP_REG);
	// 	// 	boxbme_press.push_back(_teensy_values.BOX_BME_LAST_PRESSURE_REG);
	// 	// 	boxbme_hum.push_back(_teensy_values.BOX_BME_LAST_HUM_REG);

	// 	// 	g_new_bme280_values.store(false);
	// 	// }

	// 	// static ImPlotAxisFlags yflags = ImPlotAxisFlags_AutoFit;

	// 	// /// Teensy-BME280 Plots
	// 	// if(ImGui::Button("Clear")) {
	// 	// 	bme_times.clear();
	// 	// 	localbme_hum.clear();
	// 	// 	localbme_temps.clear();
	// 	// 	localbme_press.clear();
	// 	// 	boxbme_hum.clear();
	// 	// 	boxbme_temps.clear();
	// 	// 	boxbme_press.clear();
	// 	// }
	// 	// if (ImGui::BeginTabBar("BME Plots")) {
	// 	// 	if (ImGui::BeginTabItem("Local BME")) {
	// 	// 		if (ImPlot::BeginPlot("Local BME", "time (s)", "Humidity [%]", ImVec2(-1,0),
	// 	// 			ImPlotFlags_YAxis2 | ImPlotFlags_YAxis3,	// yaxis 2 and 3 should be shown
	// 	// 			yflags,
	// 	// 			yflags,
	// 	// 			yflags,
	// 	// 			yflags,
	// 	// 			"Temperature [degC]",
	// 	// 			"Pressure [Pa]")) {
	// 	// 			if(bme_times.size() > 0) {
	// 	// 				// ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
	// 	// 				ImPlot::PlotLine("Humidity", &bme_times[0], &localbme_hum[0], bme_times.size());
						
	// 	// 				// ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
	// 	// 				//ImPlot::SetPlotYAxis(ImPlotYAxis_2);
	// 	// 				ImPlot::PlotLine("Temperature", &bme_times[0], &localbme_temps[0], bme_times.size());
						
	// 	// 				// ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
	// 	// 				//ImPlot::SetPlotYAxis(ImPlotYAxis_3);
	// 	// 				ImPlot::PlotLine("Pressure", &bme_times[0], &localbme_press[0], bme_times.size());
	// 	// 			}
	// 	// 			ImPlot::EndPlot();
	// 	// 		}
	// 	// 		ImGui::EndTabItem();
	// 	// 	}

	// 	// 	if (ImGui::BeginTabItem("Box BME")) {
	// 	// 		if (ImPlot::BeginPlot("Box BME", "time (s)", "Humidity [%]", ImVec2(-1,0),
	// 	// 			ImPlotFlags_YAxis2 | ImPlotFlags_YAxis3,	// yaxis 2 and 3 should be shown
	// 	// 			yflags,
	// 	// 			yflags,
	// 	// 			yflags,
	// 	// 			yflags,
	// 	// 			"Temperature [degC]",
	// 	// 			"Pressure [Pa]")) {
	// 	// 			if(bme_times.size() > 0) {
	// 	// 				// ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
	// 	// 				ImPlot::PlotLine("Humidity", &bme_times[0], &boxbme_hum[0], bme_times.size());
						
	// 	// 				//ImPlot::SetPlotYAxis(ImPlotYAxis_2);
	// 	// 				// ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
	// 	// 				ImPlot::PlotLine("Temperature", &bme_times[0], &boxbme_temps[0], bme_times.size());
						
	// 	// 				//ImPlot::SetPlotYAxis(ImPlotYAxis_3);
	// 	// 				// ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
	// 	// 				ImPlot::PlotLine("Pressure", &bme_times[0], &boxbme_press[0], bme_times.size());
	// 	// 			}
	// 	// 			ImPlot::EndPlot();
	// 	// 		}

	// 	// 		ImGui::EndTabItem();
	// 	// 	}


	// 	// 	ImGui::EndTabBar();
	// 	// }

	// 	// ImGui::End();
	// 	// /// !Teensy-BME280 Plots
	// 	// ////
	// 	// /// Teensy-PID Plots
	// 	// ImGui::Begin("Teensy-PID Plots"); 
	// 	// if(ImGui::Button("Clear")) {
	// 	// 		pid_times.clear();
	// 	// 		pid1_temps.clear();
	// 	// 		pid2_temps.clear();
	// 	// 		pid1_currs.clear();
	// 	// 		pid2_currs.clear();
	// 	// }
	// 	// if (ImPlot::BeginPlot("PIDs", "time (s)", "Temps [degC]", ImVec2(-1,0), 
	// 	// 	ImPlotFlags_YAxis2, 	// yaxis2 should be shown
	// 	// 	yflags,	// Flags for yaxis 1
	// 	// 	yflags,	// Flags for yaxis 2
	// 	// 	yflags,
	// 	// 	yflags,
	// 	// 	"Current [A]",
	// 	// 	"")) {
	// 	// 	if(pid_times.size() > 0) {
	// 	// 		// ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
	// 	// 		ImPlot::PlotLine("RTD1 Temp", &pid_times[0], &pid1_temps[0], pid_times.size());
	// 	// 		ImPlot::PlotLine("RTD2 Temp", &pid_times[0], &pid2_temps[0], pid_times.size());
				
	// 	// 		// ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
	// 	// 		//ImPlot::SetPlotYAxis(ImPlotYAxis_2);
	// 	// 		ImPlot::PlotLine("PID1 Current", &pid_times[0], &pid1_currs[0], pid_times.size());
	// 	// 		ImPlot::PlotLine("PID2 Current", &pid_times[0], &pid2_currs[0], pid_times.size());
				
	// 	// 	}
	// 	// 	ImPlot::EndPlot();
	// 	// }

	// 	ImGui::End();
	// 	/// !Teensy-PID Plots
	// 	////
	// 	/// SiPM Plots
	// 	ImGui::Begin("SiPM Plot");  


	// 	ImGui::End();
	// 	/// !SiPM Plots
	// 	//// !Plots

	// }

	// void GUIManager::_update_pid1() {
	// 	g_teensy_registers.store(_teensy_registers);
	// 	g_teensy_registers_changed.store(true);
	// }

} // namespace SBCQueens