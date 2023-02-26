#ifndef PIPE_H
#define PIPE_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <algorithm>
#include <functional>
#include <memory>
#include <concepts>

// C++ 3rd party includes
#include <spdlog/spdlog.h>
// my includes

namespace SBCQueens {

enum class PipeEndType { GUI, Consumer };

template<typename T>
concept HasChangedAndCallback = requires (T x) { x.Changed && x.Callback; };

// It accepts any FIFO/LIFO Queue with TypeTraits.
// DataType must be copyable, default constructable
template<template<typename, typename> class QueueType,
         typename DataType,
         typename TypeTraits,
         typename TokenType>
requires std::copyable<DataType> && std::default_initializable<DataType> && HasChangedAndCallback<DataType>
struct Pipe {
    using TypeTraits_type = TypeTraits;
    using data_type = DataType;
    using queue_type = QueueType<DataType, TypeTraits>;

    std::shared_ptr<QueueType<DataType, TypeTraits>> Queue;
    std::shared_ptr<const TokenType> GUIToken;
    std::shared_ptr<const TokenType> ThreadToken;

    explicit Pipe(std::shared_ptr<queue_type> queue) :
        Queue{queue},
        GUIToken{std::make_shared<const TokenType>(*Queue)},
        ThreadToken{std::make_shared<const TokenType>(*Queue)}
    {}

    Pipe() :
        Pipe{std::make_shared<queue_type>()}
    {}

    explicit Pipe(queue_type& queue) :
        Pipe{std::make_shared<queue_type>(queue)}
    {}
};

template<typename TPipe, PipeEndType type>
struct PipeEnd {
    using PipeData = typename TPipe::data_type;
    // PipeData is the Queue/Pipe data is being sent
    PipeData Data;
    // Pipe contains a pointer to the Queue.
    TPipe Pipe;

    // If no PipeData is provided, it will create its own
    explicit PipeEnd(TPipe pipe) :
        Data{}, Pipe{pipe}
    { }

    void send() {
        if constexpr (type == PipeEndType::GUI) {
            Pipe.Queue->try_enqueue(*Pipe.GUIToken, Data);
        } else {
            Pipe.Queue->try_enqueue(*Pipe.ThreadToken, Data);
        }
    }

    void send_if_changed() {
        if (Data.Changed) {
            Data.Changed = false;
            send();
        }
    }

    bool retrieve(PipeData& out) {
        if constexpr (type == PipeEndType::GUI) {
            return Pipe.Queue->try_dequeue_from_producer(*Pipe.ThreadToken, out);
        } else {
            return Pipe.Queue->try_dequeue_from_producer(*Pipe.GUIToken, out);
        }
    }
};

} // namespace SBCQueens


#endif