//
// Created by Popcorn on 2023-02-24.
//

#ifndef CAENTRIGGERCONFIGTAB_H
#define CAENTRIGGERCONFIGTAB_H
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

class CAENTriggerConfigTab : public Tab<SiPMAcquisitionData> {
    SiPMAcquisitionData& _data;
public:
    explicit CAENTriggerConfigTab(SiPMAcquisitionData& data) :
        Tab<SiPMAcquisitionData>("CAEN Trigger Config"), _data{data}
    {}

    ~CAENTriggerConfigTab() = default;

    void init_tab(const toml::table& cfg) override;
    void draw() override;
};

inline auto make_caen_trigger_config_tab(SiPMAcquisitionData& sipm_data) {
    return std::make_unique<CAENTriggerConfigTab>(sipm_data);
}

} // namespace SBCQueens

#endif //CAENTRIGGERCONFIGTAB_H
