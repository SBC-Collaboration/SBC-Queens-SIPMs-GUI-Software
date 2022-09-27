#include "armadillo_helpers.hpp"
#include <cstdint>

namespace SBCQueens {

arma::mat caen_event_to_armadillo(CAENEvent& evt,
	const uint32_t& num_channels) {

	if(!evt) {
		return arma::mat();
	}

	if(evt->Data == nullptr) {
		return arma::mat();
	}

	uint32_t size = evt->Data->ChSize[0];
	uint32_t n_ch = num_channels > 64 ? 64 : num_channels;

	auto m = arma::Mat<uint16_t>(evt->Data->DataChannel[0], n_ch, size);
	return arma::conv_to<arma::mat>::from(m);

}

}  // namespace SBCQueens