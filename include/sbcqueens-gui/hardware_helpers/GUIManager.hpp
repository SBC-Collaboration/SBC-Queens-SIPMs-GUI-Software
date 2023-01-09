#ifndef GUIMANAGER_H
#define GUIMANAGER_H
#pragma once

// C STD includes
#include <math.h>

// C 3rd party includes
#include <imgui.h>
#include <implot.h>

// C++ STD includes
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <functional>

// C++ 3rd party includes
#include <spdlog/spdlog.h>
#include <toml.hpp>

// My includes
#include "sbcqueens-gui/multithreading_helpers/ThreadManager.hpp"

#include "sbcqueens-gui/hardware_helpers/TeensyControllerData.hpp"
#include "sbcqueens-gui/hardware_helpers/SiPMAcquisitionData.hpp"
#include "sbcqueens-gui/hardware_helpers/SlowDAQData.hpp"

#include "sbcqueens-gui/caen_helper.hpp"
#include "sbcqueens-gui/imgui_helpers.hpp"
#include "sbcqueens-gui/implot_helpers.hpp"
#include "sbcqueens-gui/indicators.hpp"

#include "sbcqueens-gui/gui_windows/Window.hpp"
#include "sbcqueens-gui/gui_windows/ControlList.hpp"
#include "sbcqueens-gui/gui_windows/IndicatorWindow.hpp"
#include "sbcqueens-gui/gui_windows/ControlWindow.hpp"
#include "sbcqueens-gui/gui_windows/SiPMControlWindow.hpp"

namespace SBCQueens {

template<typename Pipes, typename DrawFunc>
class GUIManager : public ThreadManager<Pipes> {
    // To get the pipe interface used in the pipes.
    DrawFunc _draw_func;

    IndicatorReceiver<IndicatorNames> GeneralIndicatorReceiver;
    IndicatorReceiver<uint16_t> MultiPlotReceiver;
    IndicatorReceiver<uint8_t> SiPMPlotReceiver;

    // CAEN Controls
    using SiPMPipe_type = typename Pipes::SiPMPipe_type;
    SiPMPipe_type _sipm_pipe_end;
    SiPMAcquisitionData& _sipm_doe;

    // Teensy Pipe
    using TeensyPipe_type = typename Pipes::TeensyPipe_type;
    TeensyPipe_type _teensy_pipe_end;
    TeensyControllerData& _teensy_doe;

    // Slow DAQ pipe
    using SlowDAQ_type = typename Pipes::SlowDAQ_type;
    SlowDAQ_type _slowdaq_pipe_end;
    SlowDAQData& _slowdaq_doe;

    std::vector<Window<Pipes>> _windows;

    std::string i_run_dir;
    std::string i_run_name;

    std::vector<std::string> rtd_names;
    std::vector<std::string> sipm_names;

 public:
    GUIManager(const Pipes& p, DrawFunc&& draw_func) :
        ThreadManager<Pipes>(p), _draw_func{draw_func},
        // All of these are to have a local and faster access to them
        // inside this class.
        _sipm_pipe_end{p.SiPMPipe}, _sipm_doe{_sipm_pipe_end.Doe},
        _teensy_pipe_end{p.TeensyPipe}, _teensy_doe{_teensy_pipe_end.Doe},
        _slowdaq_pipe_end{p.SlowDAQPipe}, _slowdaq_doe{_slowdaq_pipe_end.Doe}

        // _queues(std::forward_as_tuple(queues...)),
        // GeneralIndicatorReceiver(std::get<GeneralIndicatorQueue>(_queues)),
        // MultiPlotReceiver   (std::get<MultiplePlotQueue>(_queues)),
        // SiPMPlotReceiver    (std::get<SiPMPlotQueue>(_queues)),
        // TeensyControlFac    (std::get<TeensyQueue>(_queues)),
        // CAENControlFac  (std::get<CAENQueue>(_queues)),
        // SlowDAQControlFac   (std::get<SlowDAQQueue>(_queues)),
        // indicatorWindow     (GeneralIndicatorReceiver,
        //     std::get<CAENQueue>(_queues),
        //     _teensy_doe, sdaq_state),
        // controlWindow(TeensyControlFac, _teensy_doe, CAENControlFac, _sipm_doe,
        //     SlowDAQControlFac, sdaq_state),
        // sipmControlWindow(CAENControlFac, _sipm_doe, GeneralIndicatorReceiver,
        //     _teensy_doe)
        {


        _windows.push_back(make_indicator_window(p));
        _windows.push_back(make_sipm_control_window(p));
        _windows.push_back(make_control_window(p));

        // When config_file goes out of scope, everything
        // including the daughters get cleared
        auto config_file = toml::parse_file("gui_setup.toml");
        for(auto& window: _windows) {
            window.init(config_file);
        }

        auto t_conf = config_file["Teensy"];
        if (toml::array* arr = t_conf["RTDNames"].as_array()) {
            for (toml::node& elem : *arr) {
                rtd_names.emplace_back(elem.value_or(""));
            }
        }

        if (toml::array* arr = t_conf["SiPMNames"].as_array()) {
            for (toml::node& elem : *arr) {
                sipm_names.emplace_back(elem.value_or(""));
            }
        }
    }

    GUIManager(const GUIManager&) = delete;
    GUIManager(GUIManager&&) = delete;
    GUIManager(GUIManager&) = delete;

    ~GUIManager() { }

    void operator()() {
        // _draw_func must have a recurring draw function
        // and a close function.
        _draw_func(&GUIManager<Pipes, DrawFunc>::_draw,
            &GUIManager<Pipes, DrawFunc>::closing);
    }

 private:
    void _draw() {
        static bool trg_once = true;
        if (trg_once) {
            ImPlot::StyleColorsDark();
            ImPlot::GetStyle().UseLocalTime = true;
            trg_once = false;
        }

        for(auto& window : _windows) {
            window();
        }

        //// Plots
        ImGui::Begin("Other Plots");

        // This functor updates the plots values from the queue.
        GeneralIndicatorReceiver();
        MultiPlotReceiver();
        SiPMPlotReceiver();

        const auto g_axis_flags = ImPlotAxisFlags_AutoFit;
        // /// Teensy-BME280 Plots
        if (ImGui::Button("Clear")) {
            GeneralIndicatorReceiver.ClearPlot(
                IndicatorNames::LOCAL_BME_HUMD);
            GeneralIndicatorReceiver.ClearPlot(
                IndicatorNames::LOCAL_BME_TEMPS);
            GeneralIndicatorReceiver.ClearPlot(
                IndicatorNames::LOCAL_BME_PRESS);

            GeneralIndicatorReceiver.ClearPlot(
                IndicatorNames::PFEIFFER_PRESS);
            GeneralIndicatorReceiver.ClearPlot(
                IndicatorNames::VACUUM_PRESS);
        }

        if (ImGui::BeginTabBar("Other Plots")) {
            // if (!_teensy_doe.SystemParameters.InRTDOnlyMode) {
            //     if (ImGui::BeginTabItem("Local BME")) {
            //         if (ImPlot::BeginPlot("Local BME", ImVec2(-1, 0))) {
            //             // We setup the axis
            //             ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);
            //             ImPlot::SetupAxes("time [s]", "Humidity [%]",
            //                 g_axis_flags, g_axis_flags);
            //             ImPlot::SetupAxis(ImAxis_Y3, "Pressure [Pa]",
            //                 g_axis_flags | ImPlotAxisFlags_Opposite);
            //             ImPlot::SetupAxis(ImAxis_Y2, "Temperature [degC]",
            //                 g_axis_flags | ImPlotAxisFlags_Opposite);

            //             // This one does not need an SetAxes as it takes the
            //             // default.
            //             // This functor is almost the same as calling ImPlot
            //             GeneralIndicatorReceiver.plot(
            //                 IndicatorNames::LOCAL_BME_HUMD, "Humidity");

            //             // We need to call SetAxes before ImPlot::PlotLines
            //             // to let the API know the axis of our data
            //             ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
            //             GeneralIndicatorReceiver.plot(
            //                 IndicatorNames::LOCAL_BME_TEMPS, "Temperature");

            //             ImPlot::SetAxes(ImAxis_X1, ImAxis_Y3);
            //             GeneralIndicatorReceiver.plot(
            //                 IndicatorNames::LOCAL_BME_PRESS, "Pressure");

            //             ImPlot::EndPlot();
            //         }

            //         ImGui::EndTabItem();
            //     }
            // }

            if (ImGui::BeginTabItem("Pressures")) {
                static ImPlotScale press_scale_axis = ImPlotScale_Linear;
                ImGui::CheckboxFlags("Log Axis",
                    (unsigned int*)&press_scale_axis, ImPlotScale_Log10);
                if (ImPlot::BeginPlot("Pressures", ImVec2(-1, -1))) {
                    ImPlot::SetupAxisScale(ImAxis_Y1, press_scale_axis);
                    ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);
                    ImPlot::SetupAxes("time [s]", "P [Bar]",
                        g_axis_flags, g_axis_flags);

                    GeneralIndicatorReceiver.plot(
                        IndicatorNames::PFEIFFER_PRESS, "PFEIFFER");
                    GeneralIndicatorReceiver.plot(
                        IndicatorNames::VACUUM_PRESS, "Vacuum line");

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

        if (ImGui::Button("Clear")) {
            for (uint16_t i = 0; i < _teensy_doe.SystemParameters.NumRtdBoards; i++) {
                for (uint16_t j = 0; j < _teensy_doe.SystemParameters.NumRtdsPerBoard; j++) {
                    uint16_t k = static_cast<uint16_t>(
                        i*_teensy_doe.SystemParameters.NumRtdsPerBoard +j);

                    MultiPlotReceiver.ClearPlot(k);
                }
            }
        }

        if (!_teensy_doe.SystemParameters.InRTDOnlyMode) {
            if (ImPlot::BeginPlot("PIDs", ImVec2(-1, 0))) {
                ImPlot::SetupAxes("time [s]", "Current [A]",
                    g_axis_flags, g_axis_flags);
                ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);

                GeneralIndicatorReceiver.plot(IndicatorNames::PELTIER_CURR, "Current");

                ImPlot::EndPlot();
            }
        }

        static ImPlotRect rect(0, 1, 0, 320);
        static ImPlotScale rtd_scale_axis = ImPlotScale_Linear;
        ImGui::CheckboxFlags("Log Axis",
            &rtd_scale_axis, ImPlotScale_Log10);

        if (ImPlot::BeginPlot("RTDs", ImVec2(-1, 0))) {
            ImPlot::SetupAxisScale(ImAxis_Y1, rtd_scale_axis);
            ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);
            ImPlot::SetupAxes("time [s]", "Temperature [K]",
                g_axis_flags, g_axis_flags);

            for (uint16_t i = 0; i < _teensy_doe.SystemParameters.NumRtdBoards; i++) {
                for (uint16_t j = 0; j < _teensy_doe.SystemParameters.NumRtdsPerBoard; j++) {
                    uint16_t k = static_cast<uint16_t>(
                        i*_teensy_doe.SystemParameters.NumRtdsPerBoard +j);

                    if (k < rtd_names.size()) {
                        MultiPlotReceiver.plot(k, rtd_names[k]);
                    } else {
                        MultiPlotReceiver.plot(k, std::to_string(k));
                    }
                }
            }

            rect = ImPlot::GetPlotLimits();

            ImPlot::EndPlot();
        }

        static float rtd_zoom_multiplier[2] = {0.0};
        ImGui::SliderFloat2("Zoom %", rtd_zoom_multiplier, 0.0f, 1.0f);
        // ImGui::VSliderFloat("Zoom Y%", ImVec2(18, 160),&y_rtd_zoom_multiplier, 0.0f, 1.0f);
        // ImGui::SameLine();

        if (ImPlot::BeginPlot("RTD Zoom", ImVec2(-1, 0))) {
            ImPlot::SetupAxisScale(ImAxis_Y1, rtd_scale_axis);
            ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);
            ImPlot::SetupAxes("time [s]", "Temperature [K]",
                g_axis_flags, g_axis_flags);
            ImPlot::SetupAxesLimits(
                static_cast<double>(rtd_zoom_multiplier[0])*(rect.X.Min
                    + rect.X.Max) + rect.X.Min,
                rect.X.Max,
                static_cast<double>(rtd_zoom_multiplier[1])*(rect.Y.Min
                    + rect.Y.Max) + rect.Y.Min,
                rect.Y.Max, ImGuiCond_Always);

            for (uint16_t i = 0; i < _teensy_doe.SystemParameters.NumRtdBoards; i++) {
                for (uint16_t j = 0; j < _teensy_doe.SystemParameters.NumRtdsPerBoard; j++) {
                    uint16_t k = static_cast<uint16_t>(
                        i*_teensy_doe.SystemParameters.NumRtdsPerBoard +j);

                    if (k < rtd_names.size()) {
                        MultiPlotReceiver.plot(k, rtd_names[k]);
                    } else {
                        MultiPlotReceiver.plot(k, std::to_string(k));
                    }
                }
            }

            ImPlot::EndPlot();
        }

        ImGui::End();
        /// !Teensy-PID Plots
        ////
        /// SiPM Plots


        ImGui::Begin("SiPMs Plots");

        if (ImGui::BeginTabBar("SiPMs Plots")) {
            if (ImGui::BeginTabItem("SiPM Waveforms")) {
                const size_t kCHperGroup = 8;
                const auto& model_constants
                    = CAENDigitizerModelsConstantsMap.at(_sipm_doe.Model);
                // const auto numgroups = model_constants.NumberOfGroups > 0 ?
                //     model_constants.NumberOfGroups : 1;
                // const int numchpergroup = model_constants.NumChannels / numgroups;
                // Let's work on const auto& because we really do not want to change
                // anything in these next lines
                if (ImPlot::BeginPlot("##SiPMWaveforms", ImVec2(-1, -1),
                    ImPlotFlags_NoTitle)) {
                    ImPlot::SetupAxes("time [ns]", "Counts", g_axis_flags,
                        g_axis_flags);
                    for (const auto& gp : _sipm_doe.GroupConfigs) {
                        // We need the number of groups each model has

                        // If groups is higher than 0, then it has groups
                        // otherwise each group is a channel
                        if (model_constants.NumberOfGroups > 0) {
                            // Each mask contains if the channel is enabled
                            const auto& mask = gp.AcquisitionMask;
                            // There is always at max 8 channels.
                            for (std::size_t i = 0; i < kCHperGroup; i++) {
                                if (mask & (1 << i)) {
                                    SiPMPlotReceiver.plot(
                                    static_cast<uint8_t>(kCHperGroup*gp.Number + i),
                                    "GP " +  std::to_string(gp.Number)
                                    + "CH " + std::to_string(i),
                                    PlotStyle::Line,
                                    true);
                                    ImPlot::EndPlot();
                                }
                            }
                        } else {
                            SiPMPlotReceiver.plot(gp.Number,
                                "CH " + std::to_string(gp.Number),
                                PlotStyle::Line,
                                true);
                        }
                    }
                    ImPlot::EndPlot();
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("I-V")) {
                static ImPlotScale curr_scale_axis = ImPlotScale_Linear;
                ImGui::CheckboxFlags("Current Log Axis",
                    &curr_scale_axis, ImPlotScale_Log10);

                if (ImPlot::BeginPlot("Voltage", ImVec2(-1, -1),
                    ImPlotFlags_NoTitle)) {
                    ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Time);
                    ImPlot::SetupAxisFormat(ImAxis_Y1, "%2.7f");
                    ImPlot::SetupAxes("time [EST]", "Voltage [V]", g_axis_flags,
                        g_axis_flags);
                    ImPlot::SetupAxis(ImAxis_Y2, "Current [uA]",
                            g_axis_flags | ImPlotAxisFlags_Opposite);
                    ImPlot::SetupAxisScale(ImAxis_Y2, curr_scale_axis);

                    GeneralIndicatorReceiver.plot(IndicatorNames::DMM_VOLTAGE,
                        "Voltage");

                    ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
                    GeneralIndicatorReceiver.plot(IndicatorNames::PICO_CURRENT,
                        "Current");

                    ImPlot::EndPlot();
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("G-V")) {
                if (ImGui::Button("Clear")) {
                    GeneralIndicatorReceiver.ClearPlot(
                        IndicatorNames::GAIN_VS_VOLTAGE);
                }

                if (ImPlot::BeginPlot("Voltage", ImVec2(-1, -1),
                    ImPlotFlags_NoTitle)) {
                    ImPlot::SetupAxes("Voltage [V]", "Gain [arb]", g_axis_flags,
                        g_axis_flags);

                    GeneralIndicatorReceiver.plot(
                        IndicatorNames::GAIN_VS_VOLTAGE,
                        "Gain", PlotStyle::Scatter);

                    ImPlot::EndPlot();
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    // closing is called when the GUI is manually closed.
    void closing() {
        // The only action to take is that we let the
        // Teensy Thread to close, too.
        TeensyQueue& tq = std::get<TeensyQueue&>(_queues);
        tq.try_enqueue(
            [](TeensyControllerData& oldState) {
                oldState.CurrentState = TeensyControllerStates::Closing;
                return true;
        });

        CAENQueue& cq = std::get<CAENQueue&>(_queues);
        cq.try_enqueue(
            [](CAENInterfaceData& state) {
                state.CurrentState = CAENInterfaceStates::Closing;
                return true;
        });

        SlowDAQQueue& oq = std::get<SlowDAQQueue&>(_queues);
        oq.try_enqueue(
            [](SlowDAQData& state) {
                state.PFEIFFERState = PFEIFFERSSGState::Closing;
                return true;
        });
    }
};

template<typename Pipes, typename DrawFunc>
GUIManager<Pipes, DrawFunc> make_gui_manager(const Pipes& p, DrawFunc&& df) {
    return GUIManager<Pipes, DrawFunc>(p, std::forward<DrawFunc>(df));
}

}  // namespace SBCQueens
#endif
