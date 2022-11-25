#include "sbcqueens-gui/gui_windows/IndicatorWindow.hpp"

// C STD includes
// C 3rd party includes
// C++ STD includes
// C++ 3rd party includes
#include <imgui.h>
// my includes
#include "sbcqueens-gui/implot_helpers.hpp"

namespace SBCQueens {

void IndicatorWindow::Init(const toml::table& tb) {
    _config_table = tb;
}

bool IndicatorWindow::operator()() {
    ImGui::Begin("Indicators");

    ImGui::Text("File Statistics");
    _indicator_receiver.indicator(
        IndicatorNames::SAVED_WAVEFORMS, "Saved SiPM Pulses", 200);
    ImGui::SameLine(300); ImGui::Text("[waveforms]");

    bool tmp;
    _indicator_receiver.booleanIndicator(
        IndicatorNames::DONE_DATA_TAKING, "Done data taking?",
        tmp, [=](const double& newVal) -> bool {
            return newVal > 0;
        }, 200);
    //
    ImGui::Separator();
    // Teensy
    ImGui::Text("Teensy");

    _indicator_receiver.indicator(IndicatorNames::NUM_RTD_BOARDS,
        "Number RTD Boards", _td.SystemParameters.NumRtdBoards,
        200);
    _indicator_receiver.indicator(IndicatorNames::NUM_RTDS_PER_BOARD,
        "RTDS per board", _td.SystemParameters.NumRtdsPerBoard,
        200);

    _indicator_receiver.booleanIndicator(IndicatorNames::IS_RTD_ONLY,
        "Is RTD only",
        _td.SystemParameters.InRTDOnlyMode,
        [=](const double& newVal) -> bool {
            return newVal > 0;
        }, 200);

    _indicator_receiver.indicator(IndicatorNames::PID_TEMP_ERROR,
        "Temp Error", 200);
    ImGui::SameLine(300); ImGui::Text("[K]");

    _indicator_receiver.indicator(IndicatorNames::LATEST_RTD1_TEMP,
        "RTD1 Temp", 200);
    ImGui::SameLine(300); ImGui::Text("[K]");

    _indicator_receiver.indicator(IndicatorNames::LATEST_RTD2_TEMP,
        "RTD2 Temp", 200);
    ImGui::SameLine(300); ImGui::Text("[K]");

    _indicator_receiver.indicator(IndicatorNames::LATEST_RTD3_TEMP,
        "RTD3 Temp", 200);
    ImGui::SameLine(300); ImGui::Text("[K]");

    _indicator_receiver.indicator(IndicatorNames::LATEST_PELTIER_CURR,
        "Peltier", 200);
    ImGui::SameLine(300); ImGui::Text("[A]");

    _indicator_receiver.indicator(IndicatorNames::LATEST_VACUUM_PRESS,
        "Vacuum", 200);
    ImGui::SameLine(300); ImGui::Text("[bar]");
    // End Teensy

    ImGui::Separator();
    // CAEN
    ImGui::Text("Supply indicators");

    _indicator_receiver.indicator(IndicatorNames::LATEST_DMM_VOLTAGE,
        "Latest DMM Voltage", 200, 6, NumericFormat::Scientific);
    ImGui::SameLine(300); ImGui::Text("[V]");

    _indicator_receiver.indicator(IndicatorNames::LATEST_PICO_CURRENT,
        "Latest DMM Current", 200, 6, NumericFormat::Scientific);
    ImGui::SameLine(300); ImGui::Text("[A]");

    ImGui::Separator();
    ImGui::Text("CAEN Statistics");

    _indicator_receiver.indicator(IndicatorNames::CAENBUFFEREVENTS,
        "Events in buffer", 200, 4);
    ImGui::SameLine(300); ImGui::Text("# events");

    _indicator_receiver.indicator(IndicatorNames::TRIGGERRATE,
        "Trigger Rate", 200, 4);
    ImGui::SameLine(300); ImGui::Text("[Waveforms / s]");

    ImGui::Separator();
    ImGui::Text("SiPM Statistics");

    _indicator_receiver.indicator(IndicatorNames::SPE_GAIN_MEAN,
        "1SPE Gain Mean", 200, 4);
    ImGui::SameLine(300); ImGui::Text("[arb.]");

    _indicator_receiver.indicator(IndicatorNames::SPE_EFFICIENCTY,
        "1SPE Efficiency", 200, 4);
    ImGui::SameLine(300); ImGui::Text("[%%]");

    _indicator_receiver.indicator(IndicatorNames::RISE_TIME,
        "1SPE Rise time", 200, 4);
    ImGui::SameLine(300); ImGui::Text("[sp]");

    _indicator_receiver.indicator(IndicatorNames::FALL_TIME,
        "1SPE Fall time", 200, 4);
    ImGui::SameLine(300); ImGui::Text("[sp]");

    _indicator_receiver.indicator(IndicatorNames::OFFSET,
        "1SPE Offset", 200, 4);
    ImGui::SameLine(300); ImGui::Text("[counts]");

    _indicator_receiver.indicator(IndicatorNames::INTEGRAL_THRESHOLD,
        "1SPE Integral threshold", 200, 4);
    ImGui::SameLine(300); ImGui::Text("[arb.]");

    _indicator_receiver.indicator(IndicatorNames::BREAKDOWN_VOLTAGE,
        "VBD", 200, 5);
    ImGui::SameLine(300); ImGui::Text("[V]");

    _indicator_receiver.indicator(IndicatorNames::BREAKDOWN_VOLTAGE_ERR,
        "VBD Error", 200, 3);
    ImGui::SameLine(300); ImGui::Text("[V]");

    // End CAEN
    ImGui::End();

    return true;
}

}  // namespace SBCQueens
