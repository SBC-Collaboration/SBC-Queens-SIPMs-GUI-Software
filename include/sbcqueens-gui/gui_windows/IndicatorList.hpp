#ifndef INDICATORLIST_H
#define INDICATORLIST_H
#include <implot.h>
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <tuple>

// C++ 3rd party includes
// My includes
#include "sbcqueens-gui/imgui_helpers.hpp"
#include "sbcqueens-gui/implot_helpers.hpp"

namespace SBCQueens {

constexpr static auto SiPMGUIIndicators = std::make_tuple(
	LEDIndicator<"##CAEN Connected?">("Is the CAEN Connected?"),
	LEDIndicator<"##Teensy Connected?">("Is the Teensy Connected?"),
	LEDIndicator<"##Other Connected?">("Is the SlowDAQ Connected?"),

	NumericalIndicator<"File Statistics">("Waveforms", ""),
	LEDIndicator<"Is Done Data Taking?">(""),
	NumericalIndicator<"Number RTD Boards">(""),
	NumericalIndicator<"Number RTDs per Board">(""),
	LEDIndicator<"Is RTD Only">(""),
	NumericalIndicator<"Temp Error">("K", ""),
	LEDIndicator<"Is Temp Stable?">(""),
	NumericalIndicator<"RTD 1 Temp">("K", ""),
	NumericalIndicator<"RTD 2 Temp">("K", ""),
	NumericalIndicator<"RTD 3 Temp">("K", ""),
	NumericalIndicator<"Peltier Current">("A", ""),
	NumericalIndicator<"Vacuum">("bar", ""),

	NumericalIndicator<"DMM Voltage">("V", ""),
	NumericalIndicator<"DMM Current">("A", ""),
	NumericalIndicator<"Events in buffer">("Events", ""),
	NumericalIndicator<"Trigger Rate">("Waveforms / s", ""),
	NumericalIndicator<"1SPE Gain Mean">("arb.", ""),

	// CAEN model indicators
	StringIndicator<"Model Name">("", "",
		DrawingOptions{.TextPosition = TextPositionEnum::Left}),
	NumericalIndicator<"Family Code">("", "",
		DrawingOptions{.TextPosition = TextPositionEnum::Left}),
	NumericalIndicator<"Channels">("chs", "",
		DrawingOptions{.TextPosition = TextPositionEnum::Left}),
	NumericalIndicator<"Serial Number">("", "",
		DrawingOptions{.TextPosition = TextPositionEnum::Left}),
	NumericalIndicator<"ADC Bits">("bits", "",
		DrawingOptions{.TextPosition = TextPositionEnum::Left}),
	StringIndicator<"ROC Firmware">("", "",
		DrawingOptions{.TextPosition = TextPositionEnum::Left}),
	StringIndicator<"AMC Firmware">("", "",
		DrawingOptions{.TextPosition = TextPositionEnum::Left}),
	StringIndicator<"License">("", "",
		DrawingOptions{.TextPosition = TextPositionEnum::Left})
);

template<std::size_t Index, typename Value>
constexpr auto t(Value value) {
	return value;
}

template<typename Value, std::size_t... Indices>
constexpr auto fill_same_helper(Value value, std::index_sequence<Indices...>) ->
std::array<Value, sizeof...(Indices)> {
	return {{t<Indices, Value>(value)...}};
}

template<size_t N, typename Value>
constexpr auto fill_same(Value value) {
	return fill_same_helper(value, std::make_index_sequence<N>{});
}

constexpr static auto GUIPlots = std::make_tuple(
	PlotIndicator<"I-V", 2, 2>(
		PlotOptions<2, 2>{
		.PlotType = PlotTypeEnum::Line,
		.PlotLabels = {"Current", "Voltage"},
		.PlotGroupings = {PlotGroupingsEnum::One, PlotGroupingsEnum::Two},
		.XAxisLabel = "time ",
		.XAxisUnit = "[Local Time]",
		.YAxisLabels = {"Current ", "Voltage "},
		.YAxisUnits = {"[uA]", "[V]"},
		.YAxisScales = {ImPlotScale_Linear, ImPlotScale_Linear},
		.YAxisFlags = {ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_Opposite | ImPlotAxisFlags_AutoFit}
	}, DrawingOptions{
		.Size = Size_t{-1, -1}}),

	PlotIndicator<"Group 0", 8, 1>(PlotOptions<8, 1>{
		.PlotType = PlotTypeEnum::Line,
		.PlotLabels = {"1", "2", "3", "4", "5", "6", "7", "8"},
		.PlotGroupings = fill_same<8>(PlotGroupingsEnum::One),
		.XAxisLabel = "time ",
		.XAxisUnit = "[sp]",
		.YAxisLabels = {"Counts"},
		.YAxisUnits = {""},
		.YAxisScales = {ImPlotScale_Linear},
		.YAxisFlags = {ImPlotAxisFlags_AutoFit}
	}),
	PlotIndicator<"Group 1", 8, 1>(PlotOptions<8, 1>{
		.PlotType = PlotTypeEnum::Line,
		.PlotLabels = {"9", "10", "11", "12", "13", "14", "15", "16"},
		.PlotGroupings = fill_same<8>(PlotGroupingsEnum::One),
		.XAxisLabel = "time ",
		.XAxisUnit = "[sp]",
		.YAxisLabels = {"Counts"},
		.YAxisUnits = {""},
		.YAxisScales = {ImPlotScale_Linear},
		.YAxisFlags = {ImPlotAxisFlags_AutoFit}
	}),
	PlotIndicator<"Group 2", 8, 1>(PlotOptions<8, 1>{
		.PlotType = PlotTypeEnum::Line,
		.PlotLabels = {"17", "18", "19", "20", "21", "22", "23", "24"},
		.PlotGroupings = fill_same<8>(PlotGroupingsEnum::One),
		.XAxisLabel = "time ",
		.XAxisUnit = "[sp]",
		.YAxisLabels = {"Counts"},
		.YAxisUnits = {""},
		.YAxisScales = {ImPlotScale_Linear},
		.YAxisFlags = {ImPlotAxisFlags_AutoFit}
	}),
	PlotIndicator<"Group 3", 8, 1>(PlotOptions<8, 1>{
		.PlotType = PlotTypeEnum::Line,
		.PlotLabels = {"25", "26", "27", "28", "29", "30", "31", "32"},
		.PlotGroupings = fill_same<8>(PlotGroupingsEnum::One),
		.XAxisLabel = "time ",
		.XAxisUnit = "[sp]",
		.YAxisLabels = {"Counts"},
		.YAxisUnits = {""},
		.YAxisScales = {ImPlotScale_Linear},
		.YAxisFlags = {ImPlotAxisFlags_AutoFit}
	}),
	PlotIndicator<"Group 4", 8, 1>(PlotOptions<8, 1>{
		.PlotType = PlotTypeEnum::Line,
		.PlotLabels = {"33", "34", "35", "36", "37", "38", "39", "40"},
		.PlotGroupings = fill_same<8>(PlotGroupingsEnum::One),
		.XAxisLabel = "time ",
		.XAxisUnit = "[sp]",
		.YAxisLabels = {"Counts"},
		.YAxisUnits = {""},
		.YAxisScales = {ImPlotScale_Linear},
		.YAxisFlags = {ImPlotAxisFlags_AutoFit}
	}),
    PlotIndicator<"Group 5", 8, 1>(PlotOptions<8, 1>{
            .PlotType = PlotTypeEnum::Line,
            .PlotLabels = {"41", "42", "43", "44", "45", "46", "47", "48"},
            .PlotGroupings = fill_same<8>(PlotGroupingsEnum::One),
            .XAxisLabel = "time ",
            .XAxisUnit = "[sp]",
            .YAxisLabels = {"Counts"},
            .YAxisUnits = {""},
            .YAxisScales = {ImPlotScale_Linear},
            .YAxisFlags = {ImPlotAxisFlags_AutoFit}
    }),
    PlotIndicator<"Group 6", 8, 1>(PlotOptions<8, 1>{
            .PlotType = PlotTypeEnum::Line,
            .PlotLabels = {"49", "50", "51", "52", "53", "54", "55", "56"},
            .PlotGroupings = fill_same<8>(PlotGroupingsEnum::One),
            .XAxisLabel = "time ",
            .XAxisUnit = "[sp]",
            .YAxisLabels = {"Counts"},
            .YAxisUnits = {""},
            .YAxisScales = {ImPlotScale_Linear},
            .YAxisFlags = {ImPlotAxisFlags_AutoFit}
    }),
    PlotIndicator<"Group 7", 8, 1>(PlotOptions<8, 1>{
            .PlotType = PlotTypeEnum::Line,
            .PlotLabels = {"57", "58", "59", "60", "61", "62", "63", "64"},
            .PlotGroupings = fill_same<8>(PlotGroupingsEnum::One),
            .XAxisLabel = "time ",
            .XAxisUnit = "[sp]",
            .YAxisLabels = {"Counts"},
            .YAxisUnits = {""},
            .YAxisScales = {ImPlotScale_Linear},
            .YAxisFlags = {ImPlotAxisFlags_AutoFit}
    })
);

} // namespace SBCQueens

#endif