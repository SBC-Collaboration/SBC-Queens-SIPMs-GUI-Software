#ifndef RUNTAB_H
#define RUNTAB_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
// C++ 3rd party includes
// my includes
#include "sbcqueens-gui/gui_windows/Window.hpp"

#include "sbcqueens-gui/hardware_helpers/TeensyControllerData.hpp"
#include "sbcqueens-gui/hardware_helpers/SiPMAcquisitionData.hpp"
#include "sbcqueens-gui/hardware_helpers/SlowDAQData.hpp"

namespace SBCQueens {

class RunTab
: public Tab<SiPMAcquisitionData, TeensyControllerData, SlowDAQData> {
    // CAEN Pipe
    SiPMAcquisitionData& _sipm_doe;

    // Teensy Pipe
    TeensyControllerData& _teensy_doe;

    // Slow DAQ pipe
    SlowDAQData& _slowdaq_doe;

	std::string i_run_dir = "";
    std::string i_run_name = "";

 public:
 	explicit RunTab(
    SiPMAcquisitionData& sipm_data, TeensyControllerData& teensy_data,
    SlowDAQData& slow_data) :
 		Tab<SiPMAcquisitionData, TeensyControllerData, SlowDAQData>("Run"),
        _sipm_doe{sipm_data}, _teensy_doe{teensy_data}, _slowdaq_doe{slow_data}
	{}

 	~RunTab() { }

 	void init_tab(const toml::table& cfg);
 	void draw();
};

inline auto make_run_tab(SiPMAcquisitionData& sipm_data,
    TeensyControllerData& teensy_data, SlowDAQData& slow_data) {
    return std::make_unique<RunTab>(sipm_data, teensy_data, slow_data);
}

} // namespace SBCQueens

#endif