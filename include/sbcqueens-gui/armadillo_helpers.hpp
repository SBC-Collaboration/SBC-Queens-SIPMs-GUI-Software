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
class CircularBuffer {
    arma::vec _buffer;
    arma::uword _current_index;
    arma::uword _size;
 public:
    CircularBuffer() : _buffer(),
        _current_index(0u), _size(0u) { }

    void Add(const double& new_val) {
        if (_size < N) {
            _size++;
            _buffer.resize(_size);
            _current_index = _size - 1;
        } else {
            _current_index = (_current_index + 1) % _size;
        }

        _buffer(_current_index) = new_val;
    }

    arma::uword GetOffset() {
        return _current_index;
    }

    void Clear() {
        _size = 0u;
        _current_index = 0u;

        _buffer.clear();
    }

    arma::vec GetBuffer() {
        return _buffer;
    }
};

}  // namespace SBCQueens

#endif