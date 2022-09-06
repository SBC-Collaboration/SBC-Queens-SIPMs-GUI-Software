#pragma once

/*

	This class is intended to be used to hold miscellaneous devices
	which time scales are in seconds or higher.

	Example: pressure transducers

*/

// std includes
#include <string>

// 3rd party includes


// project includes
#include "serial_helper.h"
#include "file_helpers.h"
#include "timing_events.h"

#include "indicators.h"

namespace SBCQueens {

	enum class PFEIFFERSingleGaugeSP {
		SLOW = 2, 	// 1 min
		FAST = 1, 	// 1s
		FASTER = 0	// 100ms
	};

	struct PFEIFFERSingleGaugeData {
		double Pressure;
	};

	enum class PFEIFFERSSGState {
		NullState = 0,
		Standby,
		AttemptConnection,
		Connected,
		Disconnected,
		Closing
	};

	struct OtherDevicesData {
		std::string RunDir 		= "";
		std::string RunName 	= "";

		std::string PFEIFFERPort = "";

		PFEIFFERSSGState PFEIFFERState = PFEIFFERSSGState::NullState;
		bool PFEIFFERSingleGaugeEnable = false;
		PFEIFFERSingleGaugeSP PFEIFFERSingleGaugeUpdateSpeed
			= PFEIFFERSingleGaugeSP::SLOW;
	};


	using OtherInQueueType
		= std::function < bool(OtherDevicesData&) >;

	using OtherInQueue
		= moodycamel::ReaderWriterQueue< OtherInQueueType >;

	template<typename... Queues>
	class OtherDevicesInterface {

		std::tuple<Queues&...> _queues;

		// Data of everything (doe)
		OtherDevicesData doe;

		DataFile<PFEIFFERSingleGaugeData> _pfeiffer_file;

		IndicatorSender<IndicatorNames> _plot_sender;

		// PFEIFFER Single Gauge port
		serial_ptr _pfeiffers_port;

		double _init_time;

	public:

		explicit OtherDevicesInterface(Queues&... queues) :
			_queues(std::forward_as_tuple(queues...)),
			_plot_sender(std::get<GeneralIndicatorQueue&>(_queues)) {

		}

		// No copying
		OtherDevicesInterface(const OtherDevicesInterface&) = delete;


		void operator()() {
			spdlog::info("Initializing slow DAQ thread...");
			spdlog::info("Slow DAQ components: PFEIFFERSingleGauge");

			// GUI -> Slow Daq
			OtherInQueue& guiQueueOut = std::get<OtherInQueue&>(_queues);
			auto guiQueueFunc = [&guiQueueOut]() -> SBCQueens::OtherInQueueType {

				SBCQueens::OtherInQueueType new_task;
				bool success = guiQueueOut.try_dequeue(new_task);

				if(success) {
					return new_task;
				} else {
					return [](SBCQueens::OtherDevicesData&) { return true; };
				}
			};

			auto main_loop = [&]() -> bool {
				OtherInQueueType task = guiQueueFunc();

				// If the queue does not return a valid function, this call will
				// do nothing and should return true always.
				// The tasks are essentially any GUI driven modification, example
				// setting the PID setpoints or constants
				// or an user driven reset
				if(!task(doe)) {
					spdlog::warn("Something went wrong with a command!");
				}
				// End Communication with the GUI

				if(not doe.PFEIFFERSingleGaugeEnable) {
					return true;
				}

				switch(doe.PFEIFFERState) {

					case PFEIFFERSSGState::AttemptConnection:

						connect(_pfeiffers_port, doe.PFEIFFERPort);

						if(_pfeiffers_port) {
							if(_pfeiffers_port->isOpen()) {

								_init_time = get_current_time_epoch();
								open(_pfeiffer_file,
									doe.RunDir
									+ "/" + doe.RunName
									+ "/PFEIFFERSSPressures.txt");
						 		bool s = _pfeiffer_file > 0;

								if(s) {
										spdlog::error("Failed to open files.");
										doe.PFEIFFERState =
											PFEIFFERSSGState::Standby;
									} else {
										spdlog::info(
											"Connected to PFEIFFERS with port {}",
										doe.PFEIFFERPort);

										send_msg(_pfeiffers_port, "COM," +
											std::to_string(static_cast<uint16_t> (
												doe.PFEIFFERSingleGaugeUpdateSpeed)));

										doe.PFEIFFERState =
											PFEIFFERSSGState::Connected;
									}

							} else {
								spdlog::error("Failed to connect to port {}",
									doe.PFEIFFERPort);

								doe.PFEIFFERState
									= PFEIFFERSSGState::Standby;
							}
						} else {
							// No need for a message
							doe.PFEIFFERState
								= PFEIFFERSSGState::Standby;
						}

					break;

					case PFEIFFERSSGState::Connected:

						PFEIFFER_update();

					break;

					case PFEIFFERSSGState::Disconnected:
						spdlog::warn("Manually losing connection to "
							"PFEIFFER Pressure valve with port {}",
							doe.PFEIFFERPort);
						disconnect(_pfeiffers_port);
												// Move to standby
						doe.PFEIFFERState
							= PFEIFFERSSGState::Standby;
					break;

					case PFEIFFERSSGState::Closing:
						spdlog::info("Going to close the slow DAQ thread.");
						disconnect(_pfeiffers_port);
						return false;
					break;


					case PFEIFFERSSGState::Standby:
					// do nothing
					// only way to get out of this state is by the GUI
					// starting a connection
						std::this_thread::sleep_for(
							std::chrono::milliseconds(100));
					break;

					case PFEIFFERSSGState::NullState:
					default:
						doe.PFEIFFERState = PFEIFFERSSGState::Standby;

				}

				return true;
			};

			// This will call rlf_block_time every 1 ms
			// microseconds to have better resolution or less error perhaps
			auto main_loop_block_time = make_blocking_total_timed_event(
				std::chrono::microseconds(1000), main_loop
			);

			// Actual loop!
			while(main_loop_block_time());
		}

		void PFEIFFER_update() {

			static auto retrieve_time = doe.PFEIFFERSingleGaugeUpdateSpeed ==
			PFEIFFERSingleGaugeSP::SLOW ? 60*1000 : doe.PFEIFFERSingleGaugeUpdateSpeed ==
			PFEIFFERSingleGaugeSP::FAST ? 1000 : 100;

			static auto retrieve_pids_nb = make_total_timed_event(
				std::chrono::milliseconds(retrieve_time),
				// Lambda hacking to allow the class function to be pass to
				// make_total_timed_event. Is there any other way?
				[&]() {

					auto msg = retrieve_msg<std::string>(_pfeiffers_port);

					if(msg.has_value()) {
						auto split_msg = split(msg.value(), ",");
						double pressure = std::stod(split_msg[1]);
						double dt = get_current_time_epoch() - _init_time;

						_plot_sender(IndicatorNames::PFEIFFER_PRESS, dt, pressure);
					}
				}
			);

			retrieve_pids_nb();

		}

		//https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
		std::vector<std::string> split (std::string s, std::string delimiter) {
		    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
		    std::string token;
		    std::vector<std::string> res;

		    while ((pos_end = s.find (delimiter, pos_start)) != std::string::npos) {
		        token = s.substr (pos_start, pos_end - pos_start);
		        pos_start = pos_end + delim_len;
		        res.push_back (token);
		    }

		    res.push_back (s.substr (pos_start));
		    return res;
		}


	};

} //namespace SBCQueens
