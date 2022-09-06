#pragma once

// std includes

// 3rd party includes
#include <imgui.h>

// project includes
#include "imgui_helpers.h"
#include "../TeensyControllerInterface.h"

namespace SBCQueens {

	class TeensyTabs {

		ControlLink<TeensyInQueue>& TeensyControlFac;
		TeensyControllerState& tgui_state;

public:

		TeensyTabs(ControlLink<TeensyInQueue>& tc, TeensyControllerState& ts) :
			TeensyControlFac(tc), tgui_state(ts) {}


		// Moving allowed
		TeensyTabs(TeensyTabs&&) = default;

		// No copying
		TeensyTabs(const TeensyTabs&) = delete;

		bool operator()();
	};

} // namespace SBCQueens
