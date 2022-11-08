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
const double kDMMINTERNALRES = 10e6;

// Corrects the Keithley 2000 (DMM) voltages from values obtained
// using a linear calibration from the measured voltages from the Keithley 2000,
// and the applied voltages from a recently calibrated Keithley 6487.
//
// Look under the folder 'calibration' for the data used to estimate
// the parameters.
double calibrated_dmm_voltage(const double& dmm_volt) noexcept;
// Returns the expected error from the calibration.
double calibrated_dmm_voltage_error(const double& dmm_volt) noexcept;

// Corrects for the measured voltage to estimate the actual applied voltage
// to the SiPM.
// The expected SiPM voltage is equal to the applied voltage minus the current
// times the series resistors. Vsipm = Vcal - I*Rsipm
//
// v is measured using the Keithley 2000
// i is measured using the Keithley 6487
//
// The series resistors was found using a linear fit of the Thevenin
// equivalent voltage vs the dropout using a high precision, low tempco
// known resistance (100kOhms).
// Look under the folder 'calibration' for the data used to estimate
// the resistance.
double calculate_sipm_voltage(const double& v, const double& i) noexcept;
// Returns the expected error of the SiPM Voltage. NOT IMPLEMENTED.
double calculate_sipm_voltage_error(const double& v, const double& i) noexcept;

}  // namespace SBCQueens

#endif
