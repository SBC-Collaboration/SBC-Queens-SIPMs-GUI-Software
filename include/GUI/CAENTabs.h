#pragma once

// std includes

// 3rd party includes
#include <imgui.h>

// project includes
#include "imgui_helpers.h"
#include "../CAENDigitizerInterface.h"

namespace SBCQueens {

	class CAENTabs {

		// CAEN Controls
		ControlLink<CAENQueue>& CAENControlFac;
		CAENInterfaceData& cgui_state;

public:

		CAENTabs(ControlLink<CAENQueue>& cc, CAENInterfaceData& cd) :
			CAENControlFac(cc), cgui_state(cd) { }


		// Moving allowed
		CAENTabs(CAENTabs&&) = default;

		// No copying
		CAENTabs(const CAENTabs&) = delete;


		bool operator()();

	};

} // namespace SBCQueens