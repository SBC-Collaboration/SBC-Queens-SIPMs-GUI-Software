#ifndef INDICATORWINDOW_H
#define INDICATORWINDOW_H
#pragma once

// C STD includes
// C 3rd party includes
#include <imgui.h>

// C++ STD includes
// C++ 3rd party includes
#include <toml.hpp>

// my includes
#include "sbcqueens-gui/indicators.hpp"

#include "sbcqueens-gui/gui_windows/Window.hpp"

#include "sbcqueens-gui/hardware_helpers/SiPMAcquisitionData.hpp"
#include "sbcqueens-gui/hardware_helpers/TeensyControllerData.hpp"
#include "sbcqueens-gui/hardware_helpers/SlowDAQData.hpp"

namespace SBCQueens {

template<typename Pipes>
class IndicatorWindow : public Window<Pipes> {
    // IndicatorReceiver<IndicatorNames>& _indicator_receiver;

    // CAEN Pipe
    using SiPMPipe_type = typename Pipes::SiPMPipe_type;
    SiPMAcquisitionPipeEnd<SiPMPipe_type> _sipm_pipe_end;
    SiPMAcquisitionData& _sipm_doe;

    // Teensy Pipe
    using TeensyPipe_type = typename Pipes::TeensyPipe_type;
    TeensyControllerPipeEnd<TeensyPipe_type> _teensy_pipe_end;
    TeensyControllerData& _teensy_doe;

    // Slow DAQ pipe
    using SlowDAQ_type = typename Pipes::SlowDAQ_type;
    SlowDAQPipeEnd<SlowDAQ_type> _slowdaq_pipe_end;
    SlowDAQData& _slowdaq_doe;

 public:
    explicit IndicatorWindow(const Pipes& p) :
        Window<Pipes>(p, "Indicators"),
        _sipm_pipe_end{p.SiPMPipe}, _sipm_doe(_sipm_pipe_end.Doe),
        _teensy_pipe_end{p.TeensyPipe}, _teensy_doe(_teensy_pipe_end.Doe),
        _slowdaq_pipe_end{p.SlowDAQPipe}, _slowdaq_doe(_slowdaq_pipe_end.Doe)
    {}

    ~IndicatorWindow() {}

    void init_window(const toml::table&) {
        // Nothing to do here for this window :)
    }

 private:
    void draw() {
        // ImGui::Text("File Statistics");
        // _indicator_receiver.indicator(
        //     IndicatorNames::SAVED_WAVEFORMS, "Saved SiPM Pulses", 200);
        // ImGui::SameLine(300); ImGui::Text("[waveforms]");

        // bool done_data_taking;
        // _indicator_receiver.booleanIndicator(
        //     IndicatorNames::DONE_DATA_TAKING, "Done data taking?",
        //     done_data_taking, [=](const double& newVal) -> bool {
        //         return newVal > 0;
        //     }, 200);
        // //
        // ImGui::Separator();
        // // Teensy
        // ImGui::Text("Teensy");

        // _indicator_receiver.indicator(IndicatorNames::NUM_RTD_BOARDS,
        //     "Number RTD Boards", _teensy_doe.SystemParameters.NumRtdBoards,
        //     200);
        // _indicator_receiver.indicator(IndicatorNames::NUM_RTDS_PER_BOARD,
        //     "RTDS per board", _teensy_doe.SystemParameters.NumRtdsPerBoard,
        //     200);

        // _indicator_receiver.booleanIndicator(IndicatorNames::IS_RTD_ONLY,
        //     "Is RTD only",
        //     _teensy_doe.SystemParameters.InRTDOnlyMode,
        //     [=](const double& newVal) -> bool {
        //         return newVal > 0;
        //     }, 200);

        // static double tmp_err = 1e9;
        // const double kTEMPERRTHRES = 10e-3;
        // static bool last_is_temp_stab = false, is_temp_stab = false;
        // last_is_temp_stab = is_temp_stab;
        // _indicator_receiver.indicator(IndicatorNames::PID_TEMP_ERROR,
        //     "Temp Error", tmp_err, 200);
        // ImGui::SameLine(300); ImGui::Text("[K]");

        // //
        // auto off_color = static_cast<ImVec4>(ImColor::HSV(0.0f, 0.6f, 0.6f));
        // auto on_color = static_cast<ImVec4>(ImColor::HSV(2.0f / 7.0f, 0.6f, 0.6f));

        // is_temp_stab = std::abs(tmp_err) <= kTEMPERRTHRES;
        // // did_change = (last_is_temp_stab != is_temp_stab) || did_change;

        // auto current_color = is_temp_stab ? on_color : off_color;
        // ImGui::Text("%s", "Is Temperature stabilized"); ImGui::SameLine(200);

        // ImGui::PushStyleColor(ImGuiCol_Button, current_color);
        // ImGui::PushStyleColor(ImGuiCol_ButtonHovered, current_color);
        // ImGui::PushStyleColor(ImGuiCol_ButtonActive, current_color);

        // ImGui::Button("##tmpStab", ImVec2(15, 15));

        // ImGui::PopStyleColor(3);

        // static auto update_temp_nb = make_total_timed_event(
        //         std::chrono::milliseconds(1000),
        //         // Lambda hacking to allow the class function to be pass to
        //         // make_total_timed_event. Is there any other way?
        //         [&]() {
        //         _sipm_pipe_end.try_enqueue(
        //             [=](SiPMAcquisitionData& oldState) {
        //             oldState.isTemperatureStabilized = is_temp_stab;
        //             return true;
        //         });
        //     });

        // update_temp_nb();
        // //

        // _indicator_receiver.indicator(IndicatorNames::LATEST_RTD1_TEMP,
        //     "RTD1 Temp", 200);
        // ImGui::SameLine(300); ImGui::Text("[K]");

        // _indicator_receiver.indicator(IndicatorNames::LATEST_RTD2_TEMP,
        //     "RTD2 Temp", 200);
        // ImGui::SameLine(300); ImGui::Text("[K]");

        // _indicator_receiver.indicator(IndicatorNames::LATEST_RTD3_TEMP,
        //     "RTD3 Temp", 200);
        // ImGui::SameLine(300); ImGui::Text("[K]");

        // _indicator_receiver.indicator(IndicatorNames::LATEST_PELTIER_CURR,
        //     "Peltier", 200);
        // ImGui::SameLine(300); ImGui::Text("[A]");

        // _indicator_receiver.indicator(IndicatorNames::LATEST_VACUUM_PRESS,
        //     "Vacuum", 200);
        // ImGui::SameLine(300); ImGui::Text("[bar]");
        // // End Teensy

        // ImGui::Separator();
        // // CAEN
        // ImGui::Text("Supply indicators");

        // _indicator_receiver.indicator(IndicatorNames::LATEST_DMM_VOLTAGE,
        //     "Latest DMM Voltage", 200, 6, NumericFormat::Scientific);
        // ImGui::SameLine(300); ImGui::Text("[V]");

        // _indicator_receiver.indicator(IndicatorNames::LATEST_PICO_CURRENT,
        //     "Latest DMM Current", 200, 6, NumericFormat::Scientific);
        // ImGui::SameLine(300); ImGui::Text("[A]");

        // ImGui::Separator();
        // ImGui::Text("CAEN Statistics");

        // _indicator_receiver.indicator(IndicatorNames::CAENBUFFEREVENTS,
        //     "Events in buffer", 200, 4);
        // ImGui::SameLine(300); ImGui::Text("# events");

        // _indicator_receiver.indicator(IndicatorNames::TRIGGERRATE,
        //     "Trigger Rate", 200, 4);
        // ImGui::SameLine(300); ImGui::Text("[Waveforms / s]");

        // ImGui::Separator();
        // ImGui::Text("SiPM Statistics");

        // _indicator_receiver.indicator(IndicatorNames::SPE_GAIN_MEAN,
        //     "1SPE Gain Mean", 200, 4);
        // ImGui::SameLine(300); ImGui::Text("[arb.]");

        // _indicator_receiver.indicator(IndicatorNames::SPE_EFFICIENCTY,
        //     "1SPE Efficiency", 200, 4);
        // ImGui::SameLine(300); ImGui::Text("[%%]");

        // _indicator_receiver.indicator(IndicatorNames::RISE_TIME,
        //     "1SPE Rise time", 200, 4);
        // ImGui::SameLine(300); ImGui::Text("[sp]");

        // _indicator_receiver.indicator(IndicatorNames::FALL_TIME,
        //     "1SPE Fall time", 200, 4);
        // ImGui::SameLine(300); ImGui::Text("[sp]");

        // _indicator_receiver.indicator(IndicatorNames::OFFSET,
        //     "1SPE Offset", 200, 4);
        // ImGui::SameLine(300); ImGui::Text("[counts]");

        // _indicator_receiver.indicator(IndicatorNames::INTEGRAL_THRESHOLD,
        //     "1SPE Integral threshold", 200, 4);
        // ImGui::SameLine(300); ImGui::Text("[arb.]");

        // _indicator_receiver.indicator(IndicatorNames::BREAKDOWN_VOLTAGE,
        //     "VBD", 200, 5);
        // ImGui::SameLine(300); ImGui::Text("[V]");

        // _indicator_receiver.indicator(IndicatorNames::BREAKDOWN_VOLTAGE_ERR,
        //     "VBD Error", 200, 3);
        // ImGui::SameLine(300); ImGui::Text("[V]");

        // // End CAEN
        // ImGui::End();
    }
};

template<typename Pipes>
IndicatorWindow<Pipes> make_indicator_window(const Pipes& p) {
    return IndicatorWindow<Pipes>(p);
}

}  // namespace SBCQueens
#endif
