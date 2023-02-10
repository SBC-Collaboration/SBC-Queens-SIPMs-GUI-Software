#ifndef CAENPERGROUPCONFIGTAB_H
#define CAENPERGROUPCONFIGTAB_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <memory>

// C++ 3rd party includes
// my includes
#include "sbcqueens-gui/gui_windows/Window.hpp"

#include "sbcqueens-gui/hardware_helpers/SiPMAcquisitionData.hpp"

namespace SBCQueens {

class CAENPerGroupConfigTab : public Tab<SiPMAcquisitionData> {
	SiPMAcquisitionData& _sipm_data;
 public:
	explicit CAENPerGroupConfigTab(SiPMAcquisitionData& sipm_data) :
		Tab<SiPMAcquisitionData>("CAEN Group Config"), _sipm_data{sipm_data}
	{}

 	~CAENPerGroupConfigTab() { }

 	void init_tab(const toml::table& cfg);
 	void draw();
};

inline auto make_caen_group_config_tab(SiPMAcquisitionData& sipm_data) {
	return std::make_unique<CAENPerGroupConfigTab>(sipm_data);
}

} // namespace SBCQueens

#endif