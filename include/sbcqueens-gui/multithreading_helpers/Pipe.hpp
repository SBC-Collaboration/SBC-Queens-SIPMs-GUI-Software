#ifndef PIPE_H
#define PIPE_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <functional>
#include <memory>

// C++ 3rd party includes
// my includes

namespace SBCQueens {

// It accepts any Queue with a FIFO style.
template<template<typename, typename> class QueueType, typename DataType, typename TypeTraits>
struct Pipe : public std::shared_ptr<QueueType<DataType, TypeTraits>> {
    using TypeTraits_type = TypeTraits;
    using data_type = DataType;
    using queue_type = QueueType<DataType, TypeTraits>;
};

template<class TPipe>
struct PipeEnd {
    using PipeData = typename TPipe::data_type;
    PipeData Doe;
    TPipe Pipe;

    explicit PipeEnd(TPipe pipe) : Doe{}, Pipe{pipe} {}
};

} // namespace SBCQueens


#endif