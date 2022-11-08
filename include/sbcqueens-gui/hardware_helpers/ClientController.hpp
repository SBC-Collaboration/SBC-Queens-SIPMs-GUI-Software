#ifndef CLIENTCONTROLLER_H
#define CLIENTCONTROLLER_H
#include <utility>
#include <vector>
#pragma once

/*
    This file is meant to create a monad/interface that wraps a lot of common
    logic between a server/master and client/agent. The main assumption
    is that the client is slow (O(100ms) or higher), has an initialization
    phase and does not send any commands to the master: it only sends responses.

    Example: a pressure transducer which has a setup phase then all it will
    ever do is to reply with messages (after the server ask for it) with the
    latest pressure transducer, and occasionally if the server desires
    to change its setup.

    For fast and complicated setups (several steps, feedback is needed or
    faster acquisition times) write a specialized code
*/

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <mutex>
#include <future>
#include <thread>

// C++ 3rd party includes
#include <spdlog/spdlog.h>
#include <readerwriterqueue.h>
// my includes

namespace SBCQueens {

template<typename Port, typename T>
class ClientController {
 public:
    using return_type = T;
    using queue_type = moodycamel::ReaderWriterQueue<T>;
    // Initialization function type to open the port
    using ControlFuncType = std::function<bool(Port&)>;
    // Return function type to retrieve the data from the port.
    // Return must be optional of ReturnType
    using ReturnFuncType = std::function<std::optional<T>(Port&)>;

 private:
    Port _port;
    std::string _name;
    ControlFuncType _init_func;
    ControlFuncType _close_func;

    queue_type _q;
    mutable std::mutex _port_mutex;

 public:
    explicit ClientController(const std::string& name) : _name(name) {}
    ClientController(const std::string& name, ControlFuncType&& init,
        ControlFuncType&& close)
        : _name(name), _init_func(init), _close_func(close), _q(100) {}

    // Moving allowed
    ClientController(ClientController&&) = default;
    // No copying
    ClientController(const ClientController&) = delete;

    ~ClientController() {
        _close_func(_port);
    }

    // Sets the init and close function.
    void build(ControlFuncType&& init, ControlFuncType&& close) noexcept {
        _init_func = init;
        _close_func = close;
    }

    // Builds and initializes the port
    void init(ControlFuncType&& init, ControlFuncType&& close) noexcept {
        _init_func = init;
        _close_func = close;

        if (!_port) {
            bool success = _init_func(_port);

            if (!success) {
                spdlog::error("Failed to open port under controller name {0}",
                    _name);

                // Sometimes the logic inside init would not close the
                // port, so we close it here.
                _close_func(_port);
            }
        }
    }

    // Forces to close.
    // The deconstructor also calls this function
    void close() noexcept {
        _close_func(_port);
    }

    // Holds the entire logic to connect to the port and retrieve the data.
    // A form of "lazy evaluation" leaving the connection to the port
    // until required. The code returns an optional to deal in the case the
    // port was not open or an error occur.
    std::optional<T> get(ReturnFuncType&& g) noexcept {
        if (_port) {
            return g(_port);
        } else {
            bool success = _init_func(_port);

            if (!success) {
                spdlog::error("Failed to open port under controller name {0}",
                    _name);

                // Sometimes the logic inside init would not close the
                // port, so we close it here.
                _close_func(_port);
            }

            return {};  // returns an empty optional
        }
    }

    // Holds the entire logic to connect to the port and retrieve the data.
    // A form of "lazy evaluation" leaving the connection to the port
    // until required. The code returns an optional to deal in the case the
    // port was not open or an error occur.
    std::optional<T> operator()(ReturnFuncType&& g) noexcept {
        return get(std::forward<ReturnFuncType>(g));
    }

    void async_get(ReturnFuncType&& g) noexcept {
        std::thread async_task(
            [&]() {
                std::scoped_lock guard(_port_mutex);
                std::optional<T> out = get(std::forward<ReturnFuncType>(g));
                if (out.has_value()) {
                    _q.try_enqueue(out.value());
                }
        });

        async_task.detach();
    }

    std::vector<T> async_get_values() noexcept {
        auto size = _q.size_approx();

        if (size == 0) {
            return {};
        }

        T out_single;
        std::vector<T> out(size);

        for(std::size_t i = 0; i < size; i++) {

            _q.try_dequeue(out[i]);
        }


        return out;
    }
};

}  // namespace SBCQueens

#endif
