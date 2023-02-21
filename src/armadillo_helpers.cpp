#include "sbcqueens-gui/armadillo_helpers.hpp"

// C STD includes
// C 3rd party includes
// C++ STD includes
// C++ 3rd party includes
// my includes

namespace SBCQueens {

arma::mat caen_event_to_armadillo(const CAENEvent* evt, const uint32_t& n_chs) {
	if (not evt) {
		return {};
	}

    const auto* data = evt->getData();
	uint32_t size = data->ChSize[0];
	uint32_t n_chs_c = n_chs > 64 ? 64 : n_chs;

	auto m = arma::Mat<uint16_t>(data->DataChannel[0], n_chs_c, size);
	return arma::conv_to<arma::mat>::from(m);
}

void caen_event_to_armadillo(const CAENEvent* evt, arma::mat& out,
                             const uint32_t& n_chs) {
    if (not evt) {
        return;
    }

    const auto* data = evt->getData();
    uint32_t size = data->ChSize[0];
    uint32_t n_chs_c = n_chs > 64 ? 64 : n_chs;

    auto m = arma::Mat<uint16_t>(data->DataChannel[0], n_chs_c, size);
    out = arma::conv_to<arma::mat>::from(m);
}

}  // namespace SBCQueens