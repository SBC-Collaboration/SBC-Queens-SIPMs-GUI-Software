#pragma once

// std includes
#include <cstdint>
#include <functional>
#include <numeric>
#include <thread>
#include <chrono>
#include <random>

// 3rd party includes
#include <readerwriterqueue.h>
#include <spdlog/spdlog.h>

// my includes
#include "TeensyControllerInterface.h"
#include "caen_helper.h"
#include "implot_helpers.h"
#include "include/caen_helper.h"

namespace SBCQueens {
	enum class CAENInterfaceStates {
		NullState = 0,
		Standby,
		AttemptConnection,
		OscilloscopeMode,
		StatisticsMode,
		RunMode,
		Disconnected,
		Closing
	};

	struct CAENInterfaceState {

		std::string RunDir = "";

		int PortNum = 0;
		ChannelConfig Config;

		CAEN_ptr Port = nullptr;
		CAENInterfaceStates CurrentState =
			CAENInterfaceStates::NullState;

	};

	using CAENQueueType 
		= std::function < bool(CAENInterfaceState&) >;

	using CAENQueue 
		= moodycamel::ReaderWriterQueue< CAENQueueType >;

	template<typename... Queues>
	class CAENDigitizerInterface {

		std::tuple<Queues&...> _queues;
		CAENInterfaceState state_of_everything;

		IndicatorSender<IndicatorNames> _plotSender;

		//tmp stuff
		uint16_t* data;
		size_t length;
		double* x_values, *y_values;

public:
		explicit CAENDigitizerInterface(Queues&... queues) : 
			_queues(forward_as_tuple(queues...)),
			_plotSender(std::get<SiPMsPlotQueue&>(_queues)) { }

		// No copying
		CAENDigitizerInterface(const CAENDigitizerInterface&) = delete;

		~CAENDigitizerInterface() {}

		void operator()() {

			spdlog::info("Initializing CAEN thread");

			CAEN_ptr& port = state_of_everything.Port;

			// GUI -> CAEN
			CAENQueue& guiQueueOut = std::get<CAENQueue&>(_queues);
			auto guiQueueFunc = [&guiQueueOut]() -> CAENQueueType {

				SBCQueens::CAENQueueType new_task;
				bool success = guiQueueOut.try_dequeue(new_task);

				if(success) {
					return new_task;
				} else { 
					return [](SBCQueens::CAENInterfaceState&) { return true; }; 
				}
			};

			auto main_loop = [&]() -> bool {
				CAENQueueType task = guiQueueFunc();

				// If the queue does not return a valid function, this call will
				// do nothing and should return true always.
				// The tasks are essentially any GUI driven modifcation, example
				// setting the PID setpoints or constants
				// or an user driven reset
				if(!task(state_of_everything)) {
					spdlog::warn("Something went wrong with a command!");
				}

				switch(state_of_everything.CurrentState) {
					case CAENInterfaceStates::Standby:
						// Do nothing, just sit there and look pretty
						std::this_thread::sleep_for(
							std::chrono::milliseconds(100));
					break;

					case CAENInterfaceStates::AttemptConnection:
						connect_usb(port, state_of_everything.PortNum);

						if(port) {
							spdlog::info("Connected to CAEN Digitizer!");

							auto success = reset(port);
							success |= setup(port, {state_of_everything.Config});
							success |= enable_acquisition(port);

							if(!success) {
								spdlog::warn("Failed to setup CAEN.");
								state_of_everything.CurrentState 
									= CAENInterfaceStates::Standby;
							} else {
								spdlog::info("CAEN Setup complete!");
								state_of_everything.CurrentState 
									= CAENInterfaceStates::OscilloscopeMode;
							}

						} else {
							spdlog::warn("Failed to connect to CAEN Digitizer "
								"with USB port:");
							state_of_everything.CurrentState 
								= CAENInterfaceStates::Standby;
						}

					break;

					// While in this state it shares the data with the GUI but
					// no actual file saving is happening. It essentially serves
					// as a mode in where the user can see what is happening.
					// Similar to an oscilloscope
					case CAENInterfaceStates::OscilloscopeMode:
						oscilloscope_mode();
					break;

					// Does the SiPM pulse processing but no file saving
					// is done. Serves more for the user to know
					// how things are changing.
					case CAENInterfaceStates::StatisticsMode:

						processing();

					break;

					// The real meat of this code.
					// This is intended to read the SiPM pulses, then
					// process them send the most current statistics to the GUI
					// and save them to the run file.
					case CAENInterfaceStates::RunMode:
						processing();
						save_data();
					break;

					case CAENInterfaceStates::Disconnected:

						spdlog::warn("Manually losing connection to the "
							"CAEN digitizer.");

						disconnect(port);

						state_of_everything.CurrentState 
								= CAENInterfaceStates::Standby;

					break;

					case CAENInterfaceStates::Closing:
						spdlog::info("Going to close the CAEN thread.");
						return false;
					break;

					case CAENInterfaceStates::NullState:
					default:
						// do nothing other than set to standby state
						state_of_everything.CurrentState
							= CAENInterfaceStates::Standby;
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

			// Make sure the resources are freed.
			disconnect(port);

		}

		void oscilloscope_mode() {
			CAEN_ptr& port = state_of_everything.Port;

			static auto get_data_for_gui = [&]() {
				retrieve_data(port);

				if(port->ReadData->DataSize <= 0) {
					return;
				}

				if(port->ReadData->NumEvents <= 0) {
					return;
				}

				Event event = extract_event(port, 0);

				auto buf = event.Data->DataChannel[0];
				auto size = event.Data->ChSize[0];

				x_values = new double[size];
				y_values = new double[size];

				for(uint32_t i = 0; i < size; i++) {
					x_values[i] = i*(1.0/500e6)*(1e9);
					y_values[i] = static_cast<double>(buf[i]);
				}

				_plotSender(IndicatorNames::SiPM_Plot,
					x_values,
					y_values,
					size);

				delete x_values;
				delete y_values;

				free_event(port, event);
				clear_data(port);

			};

			static auto get_data_for_gui_nb = make_blocking_total_timed_event(
				std::chrono::milliseconds(100),
				get_data_for_gui
			);

			get_data_for_gui_nb();

		}

		void processing() {
			CAEN_ptr& port = state_of_everything.Port;

			static auto extract_for_gui = [&]() {
				if(port->ReadData->DataSize <= 0) {
					return;
				}

				// For the GUI
				std::default_random_engine generator;
				std::uniform_int_distribution<uint32_t>
					distribution(0,port->ReadData->NumEvents);
				uint32_t rdm_num = distribution(generator);

				Event event = extract_event(port, rdm_num);

				auto buf = event.Data->DataChannel[0];
				auto size = event.Data->ChSize[0];

				x_values = new double[size];
				y_values = new double[size];

				for(uint32_t i = 0; i < size; i++) {
					x_values[i] = i*(1.0/500e6)*(1e9);
					y_values[i] = static_cast<double>(buf[i]);
				}

				_plotSender(IndicatorNames::SiPM_Plot,
					x_values,
					y_values,
					size);

				delete x_values;
				delete y_values;

				free_event(port, event);
			};

			static auto process_events = [&]() {
				retrieve_data(port);

				// While all of this is happening, the digitizer it taking data
				if(port->ReadData->DataSize <= 0) {
					return;
				}

				double frequency = 0.0;
				spdlog::info("There are {0} events", port->ReadData->NumEvents);
				for(uint32_t i = 0; i < port->ReadData->NumEvents; i++) {

					// Extract event i
					Event event = extract_event(port, i);

					// Process events!
					//frequency += extract_frequency(event);

					// Free event i, we no longer need the memory
					free_event(port, event);
				}

				frequency /= port->ReadData->NumEvents;
				_plotSender(IndicatorNames::FREQUENCY, frequency);

				extract_for_gui();
			};

			// static auto extract_for_gui_nb = make_total_timed_event(
			// 	std::chrono::milliseconds(500),
			// 	extract_for_gui
			// );

			static auto process_events_b = make_total_timed_event(
				std::chrono::milliseconds(345),
				process_events
			);

			process_events_b();
			// extract_for_gui_nb();

		}

		double extract_frequency(Event evt) {
			auto buf = evt.Data->DataChannel[0];
			auto size = evt.Data->ChSize[0];

			double counter = 0;
			auto offset = state_of_everything.Config.TriggerThreshold;
			for(uint32_t i = 1; i < size; i++) {
				if((buf[i] > offset) && (buf[i-1] < offset)) {
					counter++;
				}
			}

			return 500e6*counter /
				(state_of_everything.Config.RecordLength);
		}

		void save_data() {

		}
		
private:

	};
} // namespace SBCQueens