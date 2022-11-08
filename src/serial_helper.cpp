#include "sbcqueens-gui/serial_helper.hpp"

namespace SBCQueens {

void connect(serial_ptr& port, const std::string& port_name) noexcept {

    // If port not initialized
    if(!port) {
        // These are the Teensy serial
        try {

            port = std::make_unique<serial::Serial>(
                port_name,
                2500000,
                serial::Timeout::simpleTimeout(10),
                serial::bytesize_t::eightbits,
                serial::parity_t::parity_none,
                serial::stopbits_t::stopbits_one,
                serial::flowcontrol_t::flowcontrol_hardware
            );

        } catch(serial::IOException& e) {
            spdlog::error("Port not opened: {0}", e.what());
        } catch(serial::PortNotOpenedException& e) {
            spdlog::error("Port not opened: {0}", e.what());
        } catch (std::invalid_argument& e) {
            spdlog::error("Invalid argument error: {0}", e.what());
        }

    } else {

        // If port has been initialized,
        // we close it if possible and reopen!
        if(port->isOpen()) {
            port->close();
        }

        // These two functions can throw, we encapsulate them
        // and release the resources in the case it does happen
        try {
            port->setPort(port_name);
            port->open();
        } catch(serial::IOException& e) {
            spdlog::error("Port not opened: {0}", e.what());
            port.reset();
        } catch(serial::PortNotOpenedException& e) {
            spdlog::error("Port not opened: {0}", e.what());
            port.reset();
        } catch (std::invalid_argument& e) {
            spdlog::error("Invalid argument error: {0}", e.what());
            port.reset();
        }
    }

    if(port) {
        if(!port->isOpen()) {
            // If its not open, something weird happened,
            // we release the resources
            port.reset();
        } else {
            port->flush();
            port->readlines();
        }
    }
}

void connect_par(serial_ptr& port, const std::string& port_name,
    const SerialParams& sp) noexcept {
// If port not initialized
    if(!port) {
        // These are the Teensy serial
        try {

            port = std::make_unique<serial::Serial>(
                port_name,
                sp.Baudrate,
                sp.Timeout,
                sp.Bytesize,
                sp.Parity,
                sp.Stopbits,
                sp.Flowcontrol
            );

        } catch(serial::IOException& e) {
            spdlog::error("Port not opened: {0}", e.what());
            port.release();
        } catch(serial::PortNotOpenedException& e) {
            spdlog::error("Port not opened: {0}", e.what());
            port.release();
        } catch (std::invalid_argument& e) {
            spdlog::error("Invalid argument error: {0}", e.what());
            port.release();
        }

    } else {
        // If port has been initialized,
        // we close it if possible and reopen!
        if (port->isOpen()) {
            port->close();
        }

        // These two functions can throw, we encapsulate them
        // and release the resources in the case it does happen
        try {
            port->setPort(port_name);
            port->open();
        } catch(serial::IOException& e) {
            spdlog::error("Port not opened: {0}", e.what());
            port.release();
        } catch(serial::PortNotOpenedException& e) {
            spdlog::error("Port not opened: {0}", e.what());
            port.release();
        } catch (std::invalid_argument& e) {
            spdlog::error("Invalid argument error: {0}", e.what());
            port.release();
        }
    }

    if (port) {
        if (!port->isOpen()) {
            // If its not open, something weird happened,
            // we release the resources
            port.release();
        } else {
            port->flush();
            port->readlines();
        }
    }
}

void disconnect(serial_ptr& port) noexcept {
    if (port) {
        if (port->isOpen()) {
            port->flush();
            port->readlines();
            port->close();
        }

        port.release();
    }
}

void flush(serial_ptr& port) noexcept {
    if (port) {
        port->flush();
        // We need read whatever is in the buffer
        // because flush does not do that!
        port->readlines();
    }
}

bool send_msg(serial_ptr& port,
    const std::string& cmd, const std::string& ack_string) noexcept {

    if (port) {
        try {
            port->write(cmd);

            // If act_string is not empty, it reads the response
            // and check if it matches.
            // If act_string is empty, it always returns true
            if (ack_string.empty()) {
                return true;
            }

            std::string msg = port->readline(512);

            if (msg == ack_string) {
                return true;
            }

            spdlog::warn("Failed to read ack_string string ({0}) from "
                "response: {1}", ack_string, msg);
        } catch(serial::IOException& e) {
            spdlog::error("Port not open: {0}", e.what());
        } catch(serial::PortNotOpenedException& e) {
            spdlog::error("Port not open: {0}", e.what());
        } catch (std::invalid_argument& e) {
            spdlog::error("Invalid argument error: {0}", e.what());
        } catch ( ... ) {
            spdlog::error("Unknown error in send_msg");
        }
    }

    return false;
}
}  // namespace SBCQueens
