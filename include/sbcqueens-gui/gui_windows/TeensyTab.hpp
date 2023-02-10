#ifndef TEENSYTABS_H
#define TEENSYTABS_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
// C++ 3rd party includes
#include <imgui.h>

// my includes
#include "sbcqueens-gui/gui_windows/Window.hpp"

#include "sbcqueens-gui/hardware_helpers/TeensyControllerData.hpp"

namespace SBCQueens {

class TeensyTab : public Tab<TeensyControllerData> {
    // Teensy Pipe
    TeensyControllerData& _teensy_doe;

 public:
    explicit TeensyTab(TeensyControllerData& p) :
        Tab<TeensyControllerData>{"Teensy"}, _teensy_doe(p)
    {}

    ~TeensyTab() {}

    void init_tab(const toml::table& tb);
    void draw();
};

inline auto make_teensy_tab(TeensyControllerData& p) {
    return std::make_unique<TeensyTab>(p);
}

}  // namespace SBCQueens
#endif
