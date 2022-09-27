#include "GUI/IndicatorWindow.hpp"
#include "implot_helpers.hpp"
#include <imgui.h>

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
        ImGui::SameLine(300); ImGui::Text("[waveforms]");
        //
        ImGui::Separator();
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

        ImGui::Separator();
        // CAEN
        ImGui::Text("Supply indicators");

        _indicatorReceiver.indicator(IndicatorNames::LATEST_DMM_VOLTAGE,
            "Latest DMM Voltage", 200, 6, NumericFormat::Scientific);
        ImGui::SameLine(300); ImGui::Text("[V]");

        _indicatorReceiver.indicator(IndicatorNames::LATEST_PICO_CURRENT,
            "Latest DMM Voltage", 200, 6, NumericFormat::Scientific);
        ImGui::SameLine(300); ImGui::Text("[A]");

        ImGui::Separator();
        ImGui::Text("CAEN Statistics");

        _indicatorReceiver.indicator(IndicatorNames::CAENBUFFEREVENTS,
            "Events in buffer", 200, 4);
        ImGui::SameLine(300); ImGui::Text("# events");

        _indicatorReceiver.indicator(IndicatorNames::TRIGGERRATE,
            "Trigger Rate", 200, 4);
        ImGui::SameLine(300); ImGui::Text("[Waveforms / s]");

        ImGui::Separator();
        ImGui::Text("SiPM Statistics");
        _indicatorReceiver.indicator(IndicatorNames::WAVEFORM_NOISE,
            "STD noise", 200, 4);
        ImGui::SameLine(300); ImGui::Text("[counts]");

        _indicatorReceiver.indicator(IndicatorNames::SPE_GAIN_MEAN,
            "1SPE Gain Mean", 200, 4);
        ImGui::SameLine(300); ImGui::Text("[sp x counts]");

        _indicatorReceiver.indicator(IndicatorNames::SPE_EFFICIENCTY,
            "1SPE Efficiency", 200, 4);
        ImGui::SameLine(300); ImGui::Text("[%]");

        _indicatorReceiver.indicator(IndicatorNames::BREAKDOWN_VOLTAGE,
            "VBD", 200, 3);
        ImGui::SameLine(300); ImGui::Text("[V]");

        // End CAEN
        ImGui::End();

        return true;
    }

}  // namespace SBCQueens
