#pragma once

// std includes

// 3rd party includes
#include <imgui.h>
#include <toml.hpp>

// this project includes
#include "indicators.h"

namespace SBCQueens {

	class IndicatorWindow {

		toml::table _config_table;
		IndicatorReceiver<IndicatorNames>& _indicatorReceiver;

public:

		explicit IndicatorWindow(IndicatorReceiver<IndicatorNames>& ir)
			: _indicatorReceiver(ir) {}

		// Moving allowed
		IndicatorWindow(IndicatorWindow&&) = default;

		// No copying
		IndicatorWindow(const IndicatorWindow&) = delete;

		void init(const toml::table& tb) {
			_config_table = tb;
		}

		bool operator()() {

			ImGui::Begin("Indicators");

			ImGui::Text("File Statistics");
			_indicatorReceiver.indicator(IndicatorNames::SAVED_WAVEFORMS, "Saved SiPM Pulses", 200);

			// Teensy
			ImGui::Text("Teensy");

			_indicatorReceiver.indicator(IndicatorNames::NUM_RTD_BOARDS, "Number RTD Boards", 200);
			_indicatorReceiver.indicator(IndicatorNames::NUM_RTDS_PER_BOARD, "RTDS per board", 200);

			_indicatorReceiver.booleanIndicator(IndicatorNames::IS_RTD_ONLY, "Is RTD only",
				[=](const auto& newVal) {
					return newVal.x > 0;
				}, 200
			);

			_indicatorReceiver.indicator(IndicatorNames::LATEST_RTD1_TEMP, "RTD1 Temp", 200);
			ImGui::SameLine(300); ImGui::Text("[degC]");

			_indicatorReceiver.indicator(IndicatorNames::LATEST_RTD2_TEMP, "RTD2 Temp", 200);
			ImGui::SameLine(300); ImGui::Text("[degC]");

			_indicatorReceiver.indicator(IndicatorNames::LATEST_RTD3_TEMP, "RTD3 Temp", 200);
			ImGui::SameLine(300); ImGui::Text("[degC]");

			_indicatorReceiver.indicator(IndicatorNames::LATEST_Peltier_CURR, "Peltier", 200);
			ImGui::SameLine(300); ImGui::Text("[A]");

			_indicatorReceiver.indicator(IndicatorNames::LATEST_VACUUM_PRESS, "Vacuum", 200);
			ImGui::SameLine(300); ImGui::Text("[Bar]");
			// End Teensy

			// CAEN
			ImGui::Text("SiPM Statistics");

			_indicatorReceiver.indicator(IndicatorNames::CAENBUFFEREVENTS, "Events in buffer", 200, 4);
			ImGui::SameLine(300); ImGui::Text("Counts");

			_indicatorReceiver.indicator(IndicatorNames::FREQUENCY, "Signal frequency", 200, 4, NumericFormat::Scientific);
			ImGui::SameLine(300); ImGui::Text("Hz");

			_indicatorReceiver.indicator(IndicatorNames::DARK_NOISE_RATE, "Dark noise rate", 200, 3, NumericFormat::Scientific);
			ImGui::SameLine(300); ImGui::Text("Hz");

			_indicatorReceiver.indicator(IndicatorNames::GAIN, "Gain", 200, 3, NumericFormat::Scientific);
			ImGui::SameLine(300); ImGui::Text("[counts x ns]");
			// End CAEN
			ImGui::End();

			return true;
		}

	};



} // namespace SBCQueens