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
#include "sbcqueens-gui/hardware_helpers/CAENDigitizerInterface.hpp"

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

}  // namespace SBCQueens
#endif
