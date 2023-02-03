#ifndef PIPE_H
#define PIPE_H
#include <algorithm>
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <functional>
#include <memory>
#include <concepts>

// C++ 3rd party includes
// my includes

namespace SBCQueens {

template<typename T>
concept HasChangedAndCallback = requires (T x) { x.Changed && x.Callback; };

// It accepts any FIFO/LIFO Queue with TypeTraits.
// DataType must be copyable, default constructable
template<template<typename, typename> class QueueType, typename DataType, typename TypeTraits>
requires std::copyable<DataType> && std::default_initializable<DataType> && HasChangedAndCallback<DataType>
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
    // PipeData
    PipeData Data;
    // Pipe contains a pointer to the Queue.
    TPipe Pipe;

    // If no PipeData is provided, it will create its own
    explicit PipeEnd(TPipe pipe) :
        Data{}, Pipe{pipe}
    {}

    void send_if_changed() {
        if (Data.Changed) {
            if (Pipe.Queue->try_enqueue(Data)) {
                Data.Changed = false;
                // If fails, it should try again.
            }
        }
    }
};

} // namespace SBCQueens


#endif