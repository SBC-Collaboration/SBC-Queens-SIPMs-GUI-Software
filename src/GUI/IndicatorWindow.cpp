#include "GUI/IndicatorWindow.hpp"

// C STD includes
// C 3rd party includes
// C++ STD includes
// C++ 3rd party includes
// my includes

namespace SBCQueens {

    void IndicatorWindow::init(const toml::table& tb) {
        _config_table = tb;
    }

    bool IndicatorWindow::operator()() {
        ImGui::Begin("Indicators");

        ImGui::Text("File Statistics");
        _indicatorReceiver.indicator(
            IndicatorNames::SAVED_WAVEFORMS, "Saved SiPM Pulses", 200);

        // Teensy
        ImGui::Text("Teensy");

        _indicatorReceiver.indicator(IndicatorNames::NUM_RTD_BOARDS,
             "Number RTD Boards", TeensyData.SystemParameters.NumRtdBoards,
             200);
        _indicatorReceiver.indicator(IndicatorNames::NUM_RTDS_PER_BOARD,
            "RTDS per board", TeensyData.SystemParameters.NumRtdsPerBoard,
            200);

        _indicatorReceiver.booleanIndicator(IndicatorNames::IS_RTD_ONLY,
            "Is RTD only",
            TeensyData.SystemParameters.InRTDOnlyMode,
            [=](const double& newVal) -> bool {
                return newVal > 0;
            }, 200);

        _indicatorReceiver.indicator(IndicatorNames::LATEST_RTD1_TEMP,
            "RTD1 Temp", 200);
        ImGui::SameLine(300); ImGui::Text("[degC]");

        _indicatorReceiver.indicator(IndicatorNames::LATEST_RTD2_TEMP,
            "RTD2 Temp", 200);
        ImGui::SameLine(300); ImGui::Text("[degC]");

        _indicatorReceiver.indicator(IndicatorNames::LATEST_RTD3_TEMP,
            "RTD3 Temp", 200);
        ImGui::SameLine(300); ImGui::Text("[degC]");

        _indicatorReceiver.indicator(IndicatorNames::LATEST_Peltier_CURR,
            "Peltier", 200);
        ImGui::SameLine(300); ImGui::Text("[A]");

        _indicatorReceiver.indicator(IndicatorNames::LATEST_VACUUM_PRESS,
            "Vacuum", 200);
        ImGui::SameLine(300); ImGui::Text("[Bar]");
        // End Teensy

        // CAEN
        ImGui::Text("SiPM Statistics");

        _indicatorReceiver.indicator(IndicatorNames::CAENBUFFEREVENTS,
            "Events in buffer", 200, 4);
        ImGui::SameLine(300); ImGui::Text("Counts");

        _indicatorReceiver.indicator(IndicatorNames::FREQUENCY,
            "Signal frequency", 200, 4, NumericFormat::Scientific);
        ImGui::SameLine(300); ImGui::Text("Hz");

        _indicatorReceiver.indicator(IndicatorNames::DARK_NOISE_RATE,
            "Dark noise rate", 200, 3, NumericFormat::Scientific);
        ImGui::SameLine(300); ImGui::Text("Hz");

        _indicatorReceiver.indicator(IndicatorNames::GAIN,
            "Gain", 200, 3, NumericFormat::Scientific);
        ImGui::SameLine(300); ImGui::Text("[counts x ns]");
        // End CAEN
        ImGui::End();

        return true;
    }

}  // namespace SBCQueens
