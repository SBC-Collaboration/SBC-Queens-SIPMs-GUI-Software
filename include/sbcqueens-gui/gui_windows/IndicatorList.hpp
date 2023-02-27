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
	LEDIndicator<"Is RTD Only?">(""),
	NumericalIndicator<"Temp Error">("K", ""),
	LEDIndicator<"Is Temp Stable?">(""),
	NumericalIndicator<"RTD 1 Temp">("K", "", 
        DrawingOptions{.Format = "%.3f"}),
	NumericalIndicator<"RTD 2 Temp">("K", "",
        DrawingOptions{.Format = "%.3f"}),
	NumericalIndicator<"RTD 3 Temp">("K", "",
        DrawingOptions{.Format = "%.3f"}),
    NumericalIndicator<"RTD 4 Temp">("K", "",
        DrawingOptions{.Format = "%.3f"}),
    NumericalIndicator<"RTD 5 Temp">("K", "",
        DrawingOptions{.Format = "%.3f"}),
    NumericalIndicator<"RTD 6 Temp">("K", "",
        DrawingOptions{.Format = "%.3f"}),
    NumericalIndicator<"RTD 7 Temp">("K", "",
        DrawingOptions{.Format = "%.3f"}),
    NumericalIndicator<"RTD 8 Temp">("K", "",
        DrawingOptions{.Format = "%.3f"}),
    NumericalIndicator<"RTD 9 Temp">("K", "",
        DrawingOptions{.Format = "%.3f"}),
	NumericalIndicator<"Peltier Current">("A", ""),
	NumericalIndicator<"Vacuum">("mbar", ""),

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
		.Size = Size_t{-1, -1}
        }
    ),
    PlotIndicator<"Vacuum pressure", 1, 1>(
        PlotOptions<1, 1>{
            .PlotType = PlotTypeEnum::Line,
            .PlotLabels = {"Pressure"},
            .PlotGroupings = {PlotGroupingsEnum::One},
            .XAxisLabel = "time ",
            .XAxisUnit = "[Local Time]",
            .XAxisScale = ImPlotScale_Time,
            .YAxisLabels = {"Pressure"},
            .YAxisUnits = {"[mbar]"},
            .YAxisScales = {ImPlotScale_Log10},
            .YAxisFlags = {ImPlotAxisFlags_AutoFit}
        }, DrawingOptions{
            .Size = Size_t{-1, -1}
        }
    ),
    PlotIndicator<"Temperatures", 9, 1>(
        PlotOptions<9, 1>{
            .PlotType = PlotTypeEnum::Line,
            .PlotLabels = {"RTD1", "RTD2", "RTD3", "RTD4", "RTD5", "RTD6", "RTD7", "RTD8", "RTD9"},
            .PlotGroupings = fill_same<9>(PlotGroupingsEnum::One),
            .XAxisLabel = "time ",
            .XAxisUnit = "[Local Time]",
            .XAxisScale = ImPlotScale_Time,
            .YAxisLabels = {"Temperature"},
            .YAxisUnits = {"[K]"},
            .YAxisScales = {ImPlotScale_Log10},
            .YAxisFlags = {ImPlotAxisFlags_AutoFit}
        }, DrawingOptions{
            .Size = Size_t{-1, -1}
        }
    ),

	PlotIndicator<"Group 0", 8, 1>(PlotOptions<8, 1>{
		.PlotType = PlotTypeEnum::Line,
		.PlotLabels = {"0", "1", "2", "3", "4", "5", "6", "7"},
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
		.PlotLabels = {"8", "9", "10", "11", "12", "13", "14", "15"},
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
		.PlotLabels = {"16", "17", "18", "19", "20", "21", "22", "23"},
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
		.PlotLabels = {"24", "25", "26", "27", "28", "29", "30", "31"},
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
		.PlotLabels = {"32", "33", "34", "35", "36", "37", "38", "39"},
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
            .PlotLabels = {"40", "41", "42", "43", "44", "45", "46", "47"},
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
            .PlotLabels = {"48", "49", "50", "51", "52", "53", "54", "55"},
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
            .PlotLabels = {"56", "57", "58", "59", "60", "61", "62", "63"},
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
