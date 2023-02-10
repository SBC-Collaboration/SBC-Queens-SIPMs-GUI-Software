#ifndef IMPLOTHELPERS_H
#define IMPLOTHELPERS_H
#include <type_traits>
#pragma once

// C 3rd party includes
#include <implot.h>

// C++ STD includes
#include <array>
#include <memory>

// C++ 3rd party includes
#include <armadillo>

// my includes
#include "sbcqueens-gui/imgui_helpers.hpp"

namespace SBCQueens {

// Internal structure to be used for Plot drawing functions. It holds the
// plot data and is circular in nature: a new data pointer added beyond N
// loops back.
template<size_t NumPlots = 1, typename T = double>
requires std::is_integral_v<T> || std::is_floating_point_v<T>
class PlotDataBuffer {
	static_assert(NumPlots > 0, "There must be a least one plot!");

    arma::uword N;
    arma::uword _start = 0;
    arma::uword _size = 0;
    arma::uword _current_index = 0;

    std::shared_ptr<arma::Mat<T>> Data;

    typedef ImPlotPoint(*ImPlotGetterFunc)(int, void*);

    template<size_t CH>
    constexpr static auto transform() {
        return [](int idx, void* data) {
            auto myData = static_cast<PlotDataBuffer<NumPlots>*>(data);
            auto data_column = (*myData)[idx];
            return ImPlotPoint(data_column(0), data_column(CH + 1));
        };
    }

    template<std::size_t... Indices>
    constexpr static auto make_trans_func(std::index_sequence<Indices...>)
    -> std::array<ImPlotGetterFunc, sizeof...(Indices)>{
        return {{transform<Indices>()...}};
    }

    constexpr static std::array<ImPlotGetterFunc, NumPlots> generate_tans_funcs() {
        return make_trans_func(std::make_index_sequence<NumPlots>{});
    }
 public:
    // Used to interface the ImPlot API functions that end with G. They
    // require a function with the ImPlotPoint(*)(int, void*) signature,
    // but lambdas with any captures cannot be used (returns a compiler error).
    // Under the hood there are a lot of constexpr functions that generates
    // these functions at compile time which only difference is a number.
    //
    // The index is the getter function for Plot i.
    constexpr static std::array<ImPlotGetterFunc, NumPlots> TranformFunctions
        = generate_tans_funcs();

    PlotDataBuffer() = default;
    // Allocates memory constructor.
    explicit PlotDataBuffer(const arma::uword& max) :
        N(max),
        Data(std::make_shared<arma::Mat<T>>(NumPlots + 1, max, arma::fill::zeros))
    { }

    // References to another array function.
    explicit PlotDataBuffer(std::shared_ptr<arma::Mat<T>> data) :
        N(data->n_rows), Data(data)
    { }

    // Returns all the graphs points for @ i.
    // (x_i, y_{0, i}, y_{1, i}, ...)
    arma::Col<T> operator[](const arma::uword& i) {
        if (not Data) {
            return arma::Col<T>{};
        }

        arma::uword index = (i + _start) % _size;
        return Data->unsafe_col(index);
    }

    // Appends vals at the end of the circular buffer (if not full)
    // or replaces the values of the current index if full.
    template<typename... OtherTypes>
    void operator()(const OtherTypes&... vals) {
        static_assert(sizeof...(vals) == NumPlots + 1,
            "Passed number of parameters"
            "must be equal to the number of plots plus one.");

        if (not Data) {
            return;
        }

        Data->col(_current_index) = {static_cast<T>(vals)...};

        if (_size < N) {
            _size++;
            _current_index = _size == N ? 0 : _current_index + 1;
        } else {
            _start = (_start + 1) % _size;
            _current_index = _start;
        }
    }

    // Adds vals at specific index i. It ignored the circular buffer
    // conditions and does not advance them. To use this data structure
    // as a circular buffer use the operator()
    template<typename... OtherTypes>
    void add_at(const arma::uword& i, const OtherTypes&... vals) {
        static_assert(sizeof...(vals) == NumPlots + 1,
            "Passed number of parameters"
            "must be equal to the number of plots plus one.");

        if (not Data) {
            return;
        }

        Data->col(i) = {static_cast<T>(vals)...};
    }

    // Resizes the internal data buffer with new_size. Clears the data (faster)
    // if clear_data is true, otherwise, it keeps the data (slower)
    void resize(const arma::uword& new_size, const bool& clear_data = false) {
        if (not Data) {
            return;
        }

    	N = new_size;
    	if (clear_data) {
    		Data->set_size(NumPlots + 1, N);
		    _size = 0;
        	_current_index = 0;
    	} else {
    		Data->resize(NumPlots + 1, N);
    	}
    }

    // Clears the circular buffers registers. Does not clean the data.
    void clear() {
	    _size = 0;
    	_current_index = 0;
    }

    auto size() const {
    	return _size;
    }
};

using PlotGroupings_t = enum class PlotGroupingsEnum { One, Two, Three };

template<size_t NPlots = 1, size_t NYAxis = 1>
struct PlotOptions {
    static_assert(NYAxis < 4, "More than 4 axes not allowed.");
    static_assert(NPlots >= NYAxis, "Number of graphs has to be equal or higher"
        " than the number of y-axes");
    static_assert(NPlots > 0, "There must be a least one plot!");

    // Type of plot drawn
    PlotType_t PlotType = PlotTypeEnum::Line;
    //
    bool ShowAllOptions = false;

    // Labels of each plot
    std::array<std::string_view, NPlots> PlotLabels;
    // Group (in term of axes) where each plot belongs. Max 3
    std::array<PlotGroupings_t, NPlots> PlotGroupings;

    const std::string_view XAxisLabel = "x";
    std::string_view XAxisUnit = "[arb.]";
    ImPlotScale XAxisScale = ImPlotScale_Linear;
    ImPlotAxisFlags XAxisFlags = ImPlotAxisFlags_AutoFit;

    const std::array<std::string_view, NYAxis> YAxisLabels = {"y"};
    std::array<std::string_view, NYAxis> YAxisUnits = {"[arb.]"};
    std::array<ImPlotScale, NYAxis> YAxisScales;
    std::array<ImPlotAxisFlags, NYAxis> YAxisFlags;
};

template<StringLiteral list, size_t NPlots = 1, size_t NYAxis = 1>
struct PlotIndicator {
    static_assert(NYAxis < 4, "More than 4 axes not allowed.");
    static_assert(NPlots >= NYAxis, "Number of graphs has to be equal or higher"
        " than the number of y-axes");
    static_assert(NPlots > 0, "There must be a least one plot!");

    const std::string_view Label = "";
    // const std::string_view Text = "";
    // const std::string_view HelpText = "";

    // Information about the plots, their axes, and names
    PlotOptions<NPlots, NYAxis> PlotDrawOptions;
    const DrawingOptions DrawOptions;

    constexpr ~PlotIndicator() {}

    constexpr PlotIndicator() = default;
    explicit constexpr PlotIndicator(
        // const std::string_view& text, const std::string_view& help_text,
        const PlotOptions<NPlots, NYAxis>& plot_draw_opts = PlotOptions<NYAxis>{},
        const DrawingOptions& draw_opts = DrawingOptions{}) :
        Label{list.value},
        // Text{text},
        // HelpText{help_text},
        PlotDrawOptions{plot_draw_opts},
        DrawOptions{draw_opts}
    { }

    explicit constexpr PlotIndicator(const std::string_view& text) :
        PlotIndicator{text, ""}
    { }
};

// Draws the data plot_data with draw options plot at the spot this function
// is placed. Follows ImGUI/ImPlot rules.
template<StringLiteral Label, size_t NPlots, size_t NYAxis>
void Plot(const PlotIndicator<Label, NPlots, NYAxis>& plot,
    PlotDataBuffer<NPlots>& plot_data) {
    if (ImPlot::BeginPlot(Label.value, plot.DrawOptions.Size)) {
        ImPlot::SetupAxisScale(ImAxis_X1, plot.PlotDrawOptions.XAxisScale);
        ImPlot::SetupAxes(
            (std::string(plot.PlotDrawOptions.XAxisLabel) +
             std::string(plot.PlotDrawOptions.XAxisUnit)).c_str(),
            (std::string(plot.PlotDrawOptions.YAxisLabels[0]) +
             std::string(plot.PlotDrawOptions.YAxisUnits[0])).c_str(),
            plot.PlotDrawOptions.XAxisFlags,
            plot.PlotDrawOptions.YAxisFlags[0]);

        if constexpr (NYAxis == 2) {
            ImPlot::SetupAxis(ImAxis_Y2,
                              (std::string(plot.PlotDrawOptions.YAxisLabels[1]) +
                              std::string(plot.PlotDrawOptions.YAxisUnits[1])).c_str(),
                              plot.PlotDrawOptions.YAxisFlags[1]);
        } else if constexpr (NYAxis == 3) {
            ImPlot::SetupAxis(ImAxis_Y2,
                              (std::string(plot.PlotDrawOptions.YAxisLabels[1]) +
                              std::string(plot.PlotDrawOptions.YAxisUnits[1])).c_str(),
                              plot.PlotDrawOptions.YAxisFlags[1]);
            ImPlot::SetupAxis(ImAxis_Y3,
                              (std::string(plot.PlotDrawOptions.YAxisLabels[1]) +
                              std::string(plot.PlotDrawOptions.YAxisUnits[1])).c_str(),
                              plot.PlotDrawOptions.YAxisFlags[2]);
        }

        for (std::size_t i = 0; i < NPlots; i++) {
            switch (plot.PlotDrawOptions.PlotGroupings[i]) {
            case PlotGroupingsEnum::Two:
                ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
            break;
            case PlotGroupingsEnum::Three:
                ImPlot::SetAxes(ImAxis_X1, ImAxis_Y3);
            break;
            default:
                ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);
            }

            switch (plot.PlotDrawOptions.PlotType) {
            case PlotTypeEnum::Scatter:
                ImPlot::PlotScatterG(
                    std::string(plot.PlotDrawOptions.PlotLabels[i]).c_str(),
                                     PlotDataBuffer<NPlots>::TranformFunctions[i],
                                     &plot_data,
                                     plot_data.size());
            break;
            case PlotTypeEnum::Line:
            default:
                ImPlot::PlotLineG(
                    std::string(plot.PlotDrawOptions.PlotLabels[i]).c_str(),
                                  PlotDataBuffer<NPlots>::TranformFunctions[i],
                                  &plot_data,
                                  plot_data.size());
            break;
            }
        }

        ImPlot::EndPlot();
    }
}

// Helper function to get a plot drawing options from a tuple.
template<StringLiteral Label,
         size_t NPlots = 1,
         size_t NYAxis = 1,
         typename... Types>
constexpr static auto get_plot(const std::tuple<Types...>& list) {
    return std::get<PlotIndicator<Label, NPlots, NYAxis>>(list);
}

}  // namespace SBCQueens
#endif
