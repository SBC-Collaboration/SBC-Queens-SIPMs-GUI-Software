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
#include <iostream>
#include <tuple>
#include <functional>

// C++ 3rd party includes
#include <spdlog/spdlog.h>
#include <toml.hpp>

// My includes
#include "TeensyControllerInterface.hpp"
#include "CAENDigitizerInterface.hpp"
#include "SlowDAQInterface.hpp"

#include "include/caen_helper.hpp"
#include "include/imgui_helpers.hpp"
#include "include/implot_helpers.hpp"
#include "include/indicators.hpp"

#include "GUI/IndicatorWindow.hpp"
#include "GUI/ControlWindow.hpp"

namespace SBCQueens {

template<typename... QueueFuncs>
class GUIManager {
 private:
    std::tuple<QueueFuncs&...> _queues;

    IndicatorReceiver<IndicatorNames> GeneralIndicatorReceiver;
    IndicatorReceiver<uint16_t> MultiPlotReceiver;

    // Teensy Controls
    ControlLink<TeensyQueue> TeensyControlFac;
    TeensyControllerData tgui_state;

    // CAEN Controls
    ControlLink<CAENQueue> CAENControlFac;
    CAENInterfaceData cgui_state;

    // Other controls
    ControlLink<SlowDAQQueue> SlowDAQControlFac;
    SlowDAQData sdaq_state;

    std::string i_run_dir;
    std::string i_run_name;

    toml::table config_file;
    std::vector<std::string> rtd_names;
    std::vector<std::string> sipm_names;

    IndicatorWindow indicatorWindow;
    ControlWindow controlWindow;

 public:
    explicit GUIManager(QueueFuncs&... queues) :
        _queues(std::forward_as_tuple(queues...)),
        GeneralIndicatorReceiver(std::get<GeneralIndicatorQueue&>(_queues)),
        MultiPlotReceiver   (std::get<MultiplePlotQueue&>(_queues)),
        TeensyControlFac    (std::get<TeensyQueue&>(_queues)),
        CAENControlFac  (std::get<CAENQueue&>(_queues)),
        SlowDAQControlFac   (std::get<SlowDAQQueue&>(_queues)),
        indicatorWindow     (GeneralIndicatorReceiver, tgui_state, sdaq_state),
        controlWindow(TeensyControlFac, tgui_state, CAENControlFac, cgui_state,
            SlowDAQControlFac, sdaq_state) {
        // When config_file goes out of scope, everything
        // including the daughters get cleared
        config_file = toml::parse_file("gui_setup.toml");
        auto t_conf = config_file["Teensy"];
        // auto CAEN_conf = config_file["CAEN"];
        // auto file_conf = config_file["File"];

        indicatorWindow.init(config_file);
        controlWindow.init(config_file);

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

    // No copying
    GUIManager(const GUIManager&) = delete;

    ~GUIManager() { }

    void operator()() {
        ImPlot::StyleColorsDark();

        controlWindow();

        //// Plots
        ImGui::Begin("Other Plots");

        // This functor updates the plots values from the queue.
        GeneralIndicatorReceiver();
        MultiPlotReceiver();

        const auto g_axis_flags = ImPlotAxisFlags_AutoFit;
        // /// Teensy-BME280 Plots
        if (ImGui::Button("Clear")) {
            GeneralIndicatorReceiver.ClearPlot(
                IndicatorNames::LOCAL_BME_Humidity);
            GeneralIndicatorReceiver.ClearPlot(
                IndicatorNames::LOCAL_BME_Temps);
            GeneralIndicatorReceiver.ClearPlot(
                IndicatorNames::LOCAL_BME_Pressure);

            GeneralIndicatorReceiver.ClearPlot(
                IndicatorNames::PFEIFFER_PRESS);
            GeneralIndicatorReceiver.ClearPlot(
                IndicatorNames::VACUUM_PRESS);
        }

        if (ImGui::BeginTabBar("Other Plots")) {
            if (!tgui_state.SystemParameters.InRTDOnlyMode) {
                if (ImGui::BeginTabItem("Local BME")) {
                    if (ImPlot::BeginPlot("Local BME", ImVec2(-1, 0))) {
                        // We setup the axis
                        ImPlot::SetupAxes("time [s]", "Humidity [%]",
                            g_axis_flags, g_axis_flags);
                        ImPlot::SetupAxis(ImAxis_Y3, "Pressure [Pa]",
                            g_axis_flags | ImPlotAxisFlags_Opposite);
                        ImPlot::SetupAxis(ImAxis_Y2, "Temperature [degC]",
                            g_axis_flags | ImPlotAxisFlags_Opposite);

                        // This one does not need an SetAxes as it takes the
                        // default.
                        // This functor is almost the same as calling ImPlot
                        GeneralIndicatorReceiver.plot(
                            IndicatorNames::LOCAL_BME_Humidity, "Humidity");

                        // We need to call SetAxes before ImPlot::PlotLines
                        // to let the API know the axis of our data
                        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
                        GeneralIndicatorReceiver.plot(
                            IndicatorNames::LOCAL_BME_Temps, "Temperature");

                        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y3);
                        GeneralIndicatorReceiver.plot(
                            IndicatorNames::LOCAL_BME_Pressure, "Pressure");

                        ImPlot::EndPlot();
                    }

                    ImGui::EndTabItem();
                }
            }



            if (ImGui::BeginTabItem("Pressures")) {
                static ImPlotScale press_scale_axis = ImPlotScale_Linear;
                ImGui::CheckboxFlags("Log Axis",
                    (unsigned int*)&press_scale_axis, ImPlotScale_Log10);
                if (ImPlot::BeginPlot("Pressures", ImVec2(-1, 0))) {
                    ImPlot::SetupAxisScale(ImAxis_Y1, press_scale_axis);
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
            for (uint16_t i = 0; i < tgui_state.SystemParameters.NumRtdBoards; i++) {
                for (uint16_t j = 0; j < tgui_state.SystemParameters.NumRtdsPerBoard; j++) {
                    auto k = i*tgui_state.SystemParameters.NumRtdsPerBoard +j;

                    MultiPlotReceiver.ClearPlot(k);
                }
            }
        }



        if (!tgui_state.SystemParameters.InRTDOnlyMode) {
            if (ImPlot::BeginPlot("PIDs", ImVec2(-1, 0))) {
                ImPlot::SetupAxes("time [s]", "Current [A]",
                    g_axis_flags, g_axis_flags);

                ImPlot::EndPlot();
            }
        }

        static ImPlotRect rect(0, 1, 0, 320);
        static ImPlotDragToolFlags flags = ImPlotDragToolFlags_None;
        static ImPlotScale rtd_scale_axis = ImPlotScale_Linear;
        ImGui::CheckboxFlags("Log Axis",
            (unsigned int*)&rtd_scale_axis, ImPlotScale_Log10);
        if (ImPlot::BeginPlot("RTDs", ImVec2(-1, 0))) {
            ImPlot::SetupAxisScale(ImAxis_Y1, rtd_scale_axis);
            ImPlot::SetupAxes("time [s]", "Temperature [K]",
                g_axis_flags, g_axis_flags);

            for (uint16_t i = 0; i < tgui_state.SystemParameters.NumRtdBoards; i++) {
                for (uint16_t j = 0; j < tgui_state.SystemParameters.NumRtdsPerBoard; j++) {
                    auto k = i*tgui_state.SystemParameters.NumRtdsPerBoard +j;

                    if (k < rtd_names.size()) {
                        MultiPlotReceiver.plot(k, rtd_names[k]);
                    } else {
                        MultiPlotReceiver.plot(k, std::to_string(k));
                    }
                }
            }

            auto limit_rec = ImPlot::GetPlotLimits();
            rect.X.Max = limit_rec.X.Max;
            static double tmp;
            ImPlot::DragRect(0, &rect.X.Min, &rect.Y.Min, &tmp, &rect.Y.Max,
                ImVec4(1, 0, 1, 1), flags);
            rect.X.Min = rect.X.Min < limit_rec.X.Min
                ? limit_rec.X.Min : rect.X.Min;

            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("RTD Zoom", ImVec2(-1, 0))) {
            ImPlot::SetupAxisScale(ImAxis_Y1, rtd_scale_axis);
            ImPlot::SetupAxes("time [s]", "Temperature [K]",
                g_axis_flags, g_axis_flags);
            ImPlot::SetupAxesLimits(rect.X.Min, rect.X.Max,
                rect.Y.Min, rect.Y.Max, ImGuiCond_Always);

            for (uint16_t i = 0; i < tgui_state.SystemParameters.NumRtdBoards; i++) {
                for (uint16_t j = 0; j < tgui_state.SystemParameters.NumRtdsPerBoard; j++) {
                    auto k = i*tgui_state.SystemParameters.NumRtdsPerBoard +j;

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


        ImGui::Begin("SiPM Plot");
        if (ImPlot::BeginPlot("SiPM Trace", ImVec2(-1, 0))) {
            ImPlot::SetupAxes("time [ns]", "Counts",
                g_axis_flags, g_axis_flags);

            GeneralIndicatorReceiver.plot(
                IndicatorNames::SiPM_Plot_ZERO, "Plot 1", true);
            GeneralIndicatorReceiver.plot(
                IndicatorNames::SiPM_Plot_ONE, "Plot 2", true);
            GeneralIndicatorReceiver.plot(
                IndicatorNames::SiPM_Plot_TWO, "Plot 3", true);
            GeneralIndicatorReceiver.plot(
                IndicatorNames::SiPM_Plot_THREE, "Plot 4", true);

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

}  // namespace SBCQueens
#endif
