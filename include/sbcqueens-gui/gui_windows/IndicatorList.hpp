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
#include <sbcqueens-gui/imgui_helpers.hpp>

namespace SBCQueens {

constexpr static auto SiPMGUIIndicators = std::make_tuple(
	NumericalIndicator<"File Statistics">("[Waveforms]", ""),
	LEDIndicator<"Is Done Data Taking?">(""),
	NumericalIndicator<"Number RTD Boards">(""),
	NumericalIndicator<"Number RTDs per Board">(""),
	LEDIndicator<"Is RTD Only">(""),
	NumericalIndicator<"Temp Error">("[K]", ""),
	LEDIndicator<"Is Temp Stable?">(""),
	NumericalIndicator<"RTD 1 Temp">("[K]", ""),
	NumericalIndicator<"RTD 2 Temp">("[K]", ""),
	NumericalIndicator<"RTD 3 Temp">("[K]", ""),
	NumericalIndicator<"Peltier Current">("[A]", ""),
	NumericalIndicator<"Vacuum">("[bar]", ""),

	NumericalIndicator<"DMM Voltage">("[V]", ""),
	NumericalIndicator<"DMM Current">("[A]", ""),
	NumericalIndicator<"Events in buffer">("[Events]", ""),
	NumericalIndicator<"Trigger Rate">("[Waveforms / s]", ""),
	NumericalIndicator<"1SPE Gain Mean">("[arb.]", "")
);

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
		.Size = Size_t{-1, -1}})
	// PlotIndicator<"Pulses", 64, 1>(PlotOptions<64, 1>{
	// 	.PlotType = PlotTypeEnum::Line,
	// 	.PlotLabels = {"Current", "Voltage"},
	// 	.PlotGroupings = {PlotGroupingsEnum::One},
	// 	.XAxisLabel = "time [sp]",
	// 	.YAxisLabels = {"[Counts]"}
	// })
);

} // namespace SBCQueens

#endif