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

arma::mat caen_event_to_armadillo(CAENEvent& evt,
	const uint32_t& num_channels = 64);

}  // namespace SBCQueens

#endif