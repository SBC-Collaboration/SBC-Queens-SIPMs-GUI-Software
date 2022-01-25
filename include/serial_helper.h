#pragma once 

#include <memory>
#include <optional>

// Third party includes
#include <serial/serial.h>
#include <spdlog/spdlog.h>

namespace SBCQueens {
	using serial_ptr = std::unique_ptr<serial::Serial>;

	/// Connects to port, if it fails serial_ptr will point to nullptr
	void connect(serial_ptr& port, const std::string& port_name) noexcept;

	// Disconnects the port. If port is nullptr, it does nothing.
	// It does not release port.
	void disconnect(serial_ptr& port) noexcept;

	void flush(serial_ptr& port) noexcept;

	// Send message to port and if ack_string is not empty, checks if the return
	// message matches ack_string. Else, it always returns true
	bool send_msg(serial_ptr& port, 
		const std::string& cmd,
		const std::string& ack_string = "ACK;\n") noexcept;
	
	// This is a weird one, if user has to specify the return type
	// (string, int, float or double) and it will return an optional<T> of that.
	// This is to avoid error managing: ie when the port is nullptr,
	// port is not opened, etc.
	template<typename T>
	std::optional<T> retrieve_msg(serial_ptr& port) noexcept {

		if(port) {
			try{
				std::string msg = port->readline(512);
				if constexpr ( std::is_same_v<T, int>) {
					return std::make_optional(std::stoi(msg));
				} else if constexpr ( std::is_same_v<T, int32_t > ) {
					return std::make_optional(std::stol(msg));
				} else if constexpr ( std::is_same_v<T, unsigned int > ) {
					return std::make_optional(std::stoul(msg));
				} else if constexpr ( std::is_same_v<T, int64_t > ) {
					return std::make_optional(std::stoul(msg));
				} else if constexpr ( std::is_same_v<T, uint64_t > ) {
					return std::make_optional(std::stoull(msg));
				} else if constexpr ( std::is_same_v<T, float > ) {
					return std::make_optional(std::stof(msg));
				} else if constexpr ( std::is_same_v<T, double > ) {
					return std::make_optional(std::stod(msg));
				} else if constexpr ( std::is_same_v<T, long double > ) {
					return std::make_optional(std::stold(msg));
				} else {
					return std::make_optional(msg);
				}
			} catch(serial::IOException& e) {
				spdlog::error("Port not opened: {0}", e.what());
			} catch(serial::PortNotOpenedException& e) {
				spdlog::error("Port not opened: {0}", e.what());
			} catch (std::invalid_argument& e) {
				spdlog::error("Invalid argument error: {0}", e.what());
			}
		} 

		return {};
	}

	template<typename T>
	// same as above but takes a lambda to set the conversion
	std::optional<T> retrieve_msg(serial_ptr& port,
		std::function<T(const std::string&)>&& f) noexcept {

		if(port) {
			try{
				std::string msg = port->readline(512);

				return std::make_optional(f(std::forward<std::string>(msg)));

			} catch(serial::IOException& e) {
				spdlog::error("Port not opened: {0}", e.what());
			} catch(serial::PortNotOpenedException& e) {
				spdlog::error("Port not opened: {0}", e.what());
			} catch (std::invalid_argument& e) {
				spdlog::error("Invalid argument error: {0}", e.what());
			}
		}

		return {};


	}
} // namespace SBCQueens