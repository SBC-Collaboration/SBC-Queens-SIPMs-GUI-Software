#include "HardwareHelpers/Calibration.hpp"

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <cmath>
// C++ 3rd party includes
// my includes

namespace SBCQueens {

double calculate_dmm_error(const double& dmm_volt) {

	// sigma_m_prime / m_prime = sigma_m / m
	// m_prime = 1/m
	const double m = DMM_CAL_PRIME_PARS[0];
	const double b = DMM_CAL_PRIME_PARS[1];

	const double m_var = DMM_CAL_STD[0]*DMM_CAL_STD[0];
	const double b_var = DMM_CAL_STD[1]*DMM_CAL_STD[1];

	return sqrt(m_var*dmm_volt*dmm_volt + b_var + 2*DMM_CAL_COV*dmm_volt);


}

}  // namespace SBCQueens