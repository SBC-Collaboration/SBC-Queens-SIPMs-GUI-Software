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

const double DMM_CAL_PRIME_PARS[] = {1.00000790e+00,  -7.03796763e-06};
const double DMM_CAL_VAR[] = { 5.79188926e-10, 6.74259920e-07};
const double kDMMCALCOV = -1.71253999e-08;
const double kRINTERNAL = 30642;  // Ohms
const double kRINTERNALSTD = 6;  // Ohms

double calculate_dmm_voltage(const double& dmm_volt);
double calculate_dmm_voltage_error(const double& dmm_volt);
double calculate_sipm_voltage(const double& volt, const double& current);
double calculate_sipm_voltage_error(const double& volt, const double& current);

}  // namespace SBCQueens

#endif
