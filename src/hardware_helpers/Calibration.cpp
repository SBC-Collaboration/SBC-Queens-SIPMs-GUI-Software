#include "sbcqueens-gui/hardware_helpers/Calibration.hpp"

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <cmath>
// C++ 3rd party includes
// my includes

namespace SBCQueens {

double calibrated_dmm_voltage(const double& dmm_volt) noexcept {
	const double m = DMM_CAL_PRIME_PARS[0];
	const double b = DMM_CAL_PRIME_PARS[1];

	return m*dmm_volt + b;
}

double calibrated_dmm_voltage_error(const double& dmm_volt) noexcept {
	const double m_var = DMM_CAL_VAR[0];
	const double b_var = DMM_CAL_VAR[1];

	return std::sqrt(m_var*dmm_volt*dmm_volt + b_var + 2*kDMMCALCOV*dmm_volt);
}

double calculate_sipm_voltage(const double& v, const double& i) noexcept {
	return calibrated_dmm_voltage(v) - i*kRINTERNAL;
}

double calculate_sipm_voltage_error(const double& v, const double& i) noexcept {
	return 0.0;
}

}  // namespace SBCQueens
