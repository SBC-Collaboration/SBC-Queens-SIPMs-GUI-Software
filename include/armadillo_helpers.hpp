#ifndef ARMADILLOHELPERS_H
#define ARMADILLOHELPERS_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD include
// C++ 3rd party includes
#include <armadillo>
// my includes
#include "caen_helper.hpp"

namespace SBCQueens {

// Translates a CAENEvent into a matrix where each row is a waveform
// corresponding to channel CH.
//
// It does not take into account disabled channels. For example, If channels
// 1, 3 and 4 are used then n_chs should be 4 and it will return the data
// from ch 2 filled with all 0s.
//
// Returns an empty mat if there is no event or data.
arma::mat caen_event_to_armadillo(CAENEvent& evt, const uint32_t& n_chs = 64);

}  // namespace SBCQueens

#endif