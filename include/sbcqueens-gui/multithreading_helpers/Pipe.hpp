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
struct Pipe {
    std::shared_ptr<QueueType<DataType, TypeTraits>> Queue;
    using TypeTraits_type = TypeTraits;
    using data_type = DataType;
    using queue_type = QueueType<DataType, TypeTraits>;

    explicit Pipe() : Queue{std::make_shared<queue_type>()} {}
    explicit Pipe(std::shared_ptr<queue_type> queue) : Queue{queue} {}
    explicit Pipe(queue_type& queue) : Queue{std::make_shared<queue_type>(queue)} {}
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