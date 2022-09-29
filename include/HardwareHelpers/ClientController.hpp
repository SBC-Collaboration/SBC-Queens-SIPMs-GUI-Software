#ifndef CLIENTCONTROLLER_H
#define CLIENTCONTROLLER_H
#include <utility>
#pragma once

/*
    This file is meant to create a monad/interface that wraps a lot of common
    logic between a server/master and client/agent. The main assumption
    is that the client is slow (O(100ms) or higher), has an initialization
    phase and does not send any commands to the master only responses.

    Example: a pressure transducer which has a setup phase then all it will
    ever do is to reply with messages (after the server ask for it) with the
    latest pressure transducer, and occasionally if the server desires
    to change its setup.

    For fast and complicated setups (several steps, feedback is needed or
    faster acquisition times) wire an specialized code
*/

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <functional>
#include <memory>
#include <optional>
#include <string>

// C++ 3rd party includes
#include <spdlog/spdlog.h>
// my includes

namespace SBCQueens {

template<typename Port, typename T>
class ClientController {
 public:
    // Initialization function type to open the port
    using ControlFuncType = std::function<bool(Port&)>;
    // Return function type to retrieve the data from the port.
    // Return must be optional of ReturnType
    using ReturnFuncType = std::function<std::optional<T>(Port&)>;

 private:
    Port _port;
    std::string _name;
    ControlFuncType _initFunc;
    ControlFuncType _closeFunc;

 public:
    explicit ClientController(const std::string& name) : _name(name) {}
    ClientController(const std::string& name, ControlFuncType&& init,
        ControlFuncType&& close)
        : _name(name), _initFunc(init), _closeFunc(close) {}

    ~ClientController() {
        _closeFunc(_port);
    }

    void init(ControlFuncType&& init, ControlFuncType&& close) noexcept {
        _initFunc = init;
        _closeFunc = close;
    }

    void close() noexcept {
        _closeFunc(_port);
    }

    // Holds the entire logic to connect to the port and retrieve the data.
    // A form of "lazy evaluation" leaving the connection to the port
    // until required. The code returns an optional to deal in the case the
    // port is was not open or an error occur..
    std::optional<T> get(ReturnFuncType&& g) noexcept {
        if (_port) {
            return g(_port);
        } else {
            bool success = _initFunc(_port);

            if (!success) {
                spdlog::error("Failed to open port under controller name {0}",
                    _name);

                // Sometimes the logic inside init would not close the
                // port, so we close it here.
                _closeFunc(_port);

            }

            return {};  // returns an empty optional
        }
    }

    std::optional<T> operator()(ReturnFuncType&& g) noexcept {
        return get(std::forward<ReturnFuncType>(g));
    }
};

}  // namespace SBCQueens

#endif
