//
// Created by Popcorn on 2023-02-24.
//
#include "sbcqueens-gui/gui_windows/CAENTriggerConfigTab.hpp"

// C STD includes
// C 3rd party includes
// C++ STD includes
// C++ 3rd party includes
// my includes
#include "sbcqueens-gui/gui_windows/ControlList.hpp"

namespace SBCQueens {

void CAENTriggerConfigTab::init_tab(const toml::table& cfg) {
    auto caen_cfg = cfg["CAEN"];
    _data.GlobalConfig.MajorityLevel = caen_cfg["MajorityLevel"].value_or<uint32_t>(0);
    _data.GlobalConfig.MajorityCoincidenceWindow = caen_cfg["MajorityCoincidenceWindow"].value_or<uint32_t>(0);
}

void CAENTriggerConfigTab::draw() {
    constexpr auto majority_level_ctr  =
            get_control<ControlTypes::InputUINT32, "Majority Level">(SiPMGUIControls);
    draw_control(majority_level_ctr,
         _data,
         _data.GlobalConfig.MajorityLevel,
         ImGui::IsItemEdited,
        // Callback when IsItemEdited !
         [&](SiPMAcquisitionData& twin) {
             twin.GlobalConfig.MajorityLevel = _data.GlobalConfig.MajorityLevel;
         }
    );

    constexpr auto majority_coincidence_window_ctr  =
            get_control<ControlTypes::InputUINT32, "Majority Coincidence Window">(SiPMGUIControls);
    draw_control(majority_coincidence_window_ctr,
         _data,
         _data.GlobalConfig.MajorityCoincidenceWindow,
         ImGui::IsItemEdited,
        // Callback when IsItemEdited !
         [&](SiPMAcquisitionData& twin) {
             twin.GlobalConfig.MajorityCoincidenceWindow = _data.GlobalConfig.MajorityCoincidenceWindow;
         }
    );

}

} // namespace SBCQueens