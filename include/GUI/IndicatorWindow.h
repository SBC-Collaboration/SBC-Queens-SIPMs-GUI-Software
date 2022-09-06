#pragma once

// std includes

// 3rd party includes
#include <imgui.h>
#include <toml.hpp>

// this project includes
#include "indicators.h"

#include "../TeensyControllerInterface.h"
#include "../OtherDevicesInterface.h"

namespace SBCQueens {

	class IndicatorWindow {

		toml::table _config_table;
		IndicatorReceiver<IndicatorNames>& _indicatorReceiver;
		TeensyControllerState& TeensyData;
		OtherDevicesData& OtherDevData;

public:

		IndicatorWindow(IndicatorReceiver<IndicatorNames>& ir,
			TeensyControllerState& td, OtherDevicesData& od)
			: _indicatorReceiver(ir), TeensyData(td), OtherDevData(od) {}

		// Moving allowed
		IndicatorWindow(IndicatorWindow&&) = default;

		// No copying
		IndicatorWindow(const IndicatorWindow&) = delete;

		void init(const toml::table& tb);

		bool operator()();

	};



} // namespace SBCQueens