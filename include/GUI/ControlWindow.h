#pragma once

// std includes

// 3rd party includes
#include <imgui.h>
#include <toml.hpp>

// this project includes
#include "imgui_helpers.h"
#include "../TeensyControllerInterface.h"
#include "../CAENDigitizerInterface.h"
#include "../OtherDevicesInterface.h"

#include "TeensyTabs.h"
#include "CAENTabs.h"

namespace SBCQueens {

	class ControlWindow {

		toml::table _config_table;

		// Teensy Controls
		ControlLink<TeensyQueue>& TeensyControlFac;
		TeensyControllerData& tgui_state;

		// CAEN Controls
		ControlLink<CAENQueue>& CAENControlFac;
		CAENInterfaceData& cgui_state;

		//
		ControlLink<OtherQueue>& SlowDAQControlFac;
		OtherDevicesData& other_state;

		std::string i_run_dir;
		std::string i_run_name;

		TeensyTabs ttabs;
		CAENTabs ctabs;

public:

		ControlWindow(ControlLink<TeensyQueue>& tc, TeensyControllerData& ts,
			ControlLink<CAENQueue>& cc, CAENInterfaceData& cd,
			ControlLink<OtherQueue>& oc, OtherDevicesData& od) :
			 TeensyControlFac(tc), tgui_state(ts),
			 CAENControlFac(cc), cgui_state(cd),
			 SlowDAQControlFac(oc), other_state(od),
			 ttabs(tc, ts), ctabs(cc, cd)
		{}

		// Moving allowed
		ControlWindow(ControlWindow&&) = default;

		// No copying
		ControlWindow(const ControlWindow&) = delete;

		void init(const toml::table& tb);

		bool operator()();

private:
		void _gui_controls_tab();

	};

} // namespace SBCQueens