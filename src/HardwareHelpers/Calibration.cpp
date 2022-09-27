#include "HardwareHelpers/Calibration.hpp"

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <cmath>
// C++ 3rd party includes
// my includes

namespace SBCQueens {

double calculate_dmm_voltage(const double& dmm_volt) {
	const double m = DMM_CAL_PRIME_PARS[0];
	const double b = DMM_CAL_PRIME_PARS[1];

	return m*dmm_volt + b;
}

double calculate_dmm_voltage_error(const double& dmm_volt) {
	const double m_var = DMM_CAL_VAR[0];
	const double b_var = DMM_CAL_VAR[1];

	return sqrt(m_var*dmm_volt*dmm_volt + b_var + 2*kDMMCALCOV*dmm_volt);
}

double calculate_sipm_voltage(const double& volt, const double& current) {
	return 0.0;
}

double calculate_sipm_voltage_error(const double& volt, const double& current) {
	return 0.0;
}

}  // namespace SBCQueens
