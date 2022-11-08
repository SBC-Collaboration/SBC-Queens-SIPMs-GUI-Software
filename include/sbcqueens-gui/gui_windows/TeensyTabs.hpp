#ifndef TEENSYTABS_H
#define TEENSYTABS_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
// C++ 3rd party includes
#include <imgui.h>

// my includes
#include "sbcqueens-gui/imgui_helpers.hpp"
#include "sbcqueens-gui/hardware_helpers/TeensyControllerInterface.hpp"

namespace SBCQueens {

class TeensyTabs {
    ControlLink<TeensyQueue>& TeensyControlFac;
    TeensyControllerData& tgui_state;

 public:
    TeensyTabs(ControlLink<TeensyQueue>& tc, TeensyControllerData& ts) :
        TeensyControlFac(tc), tgui_state(ts) {}


    // Moving allowed
    TeensyTabs(TeensyTabs&&) = default;

    // No copying
    TeensyTabs(const TeensyTabs&) = delete;

    bool operator()();
};

}  // namespace SBCQueens
#endif
