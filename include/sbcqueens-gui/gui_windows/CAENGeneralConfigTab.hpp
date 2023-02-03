#ifndef CAENTABS_H
#define CAENTABS_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
// C++ 3rd party includes
#include <imgui.h>

// my includes
#include "sbcqueens-gui/imgui_helpers.hpp"

#include "sbcqueens-gui/gui_windows/Window.hpp"
#include "sbcqueens-gui/gui_windows/ControlList.hpp"

#include "sbcqueens-gui/hardware_helpers/SiPMAcquisitionData.hpp"

// TODO(Hector): add CAEN per Channel config tab

namespace SBCQueens {

class CAENGeneralConfigTab : public Tab<SiPMAcquisitionData> {
    SiPMAcquisitionData& _sipm_doe;

 public:
    explicit CAENGeneralConfigTab(SiPMAcquisitionData& p) :
        Tab<SiPMAcquisitionData>{"CAEN Global"},
        _sipm_doe(p)
    { }

    ~CAENGeneralConfigTab() {}
 private:
    void init_tab(const toml::table& tb);
    void draw();
};

inline auto make_caen_general_config_tab(SiPMAcquisitionData& p) {
    return std::make_unique<CAENGeneralConfigTab>(p);
}

}  // namespace SBCQueens
#endif
