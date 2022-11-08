#ifndef ARMADILLOHELPERS_H
#define ARMADILLOHELPERS_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD include
// C++ 3rd party includes
#include <armadillo>
// my includes
#include "sbcqueens-gui/caen_helper.hpp"

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

template<size_t N>
class CircularBuffer : public arma::vec::fixed<N> {
    arma::uword _current_index;
 public:
    CircularBuffer() : arma::vec::fixed<N>(arma::fill::randn),
        _current_index(0u) { }

    void operator()(const double& new_val) {
        arma::vec::fixed<N>::operator()(_current_index) = new_val;
        _current_index++;
        _current_index = _current_index % N;
    }
};

}  // namespace SBCQueens

#endif