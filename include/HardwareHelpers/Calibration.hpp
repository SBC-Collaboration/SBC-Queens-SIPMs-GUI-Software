#ifndef CALIBRATION_H
#define CALIBRATION_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <utility>
// C++ 3rd party includes
// my includes

namespace SBCQueens {

const double DMM_CAL_PRIME_PARS[] = {1.00006224e+00,  2.47822383e-05};
const double DMM_CAL_STD[] = { 4.842252035995647e-06, 2.6919326624132015e-05};
const double DMM_CAL_COV = -1.11407167e-10;

double calculate_dmm_error(const double& dmm_volt);

}  // namespace SBCQueens

#endif