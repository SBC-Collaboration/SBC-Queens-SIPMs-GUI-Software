#pragma once

// std includes
#include <cstdint>
#include <functional>
#include <memory>
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
#include "deps/spdlog/include/spdlog/spdlog.h"
#include "implot_helpers.h"
#include "include/caen_helper.h"
#include "include/implot_helpers.h"
#include "include/indicators.h"
#include "include/timing_events.h"


// SBCQueens::BlockingTotalTimeEvent<std::function<bool(
// SBCQueens::CAENDigitizerInterface<moodycamel::ConcurrentQueue<
// SBCQueens::IndicatorVector<SBCQueens::IndicatorNames, double>,
// moodycamel::ConcurrentQueueDefaultTraits>,
// moodycamel::ReaderWriterQueue<std::function<bool(SBCQueens::CAENInterfaceState&)>, 512> >*)>,
// std::chrono::duration<long long int, std::ratio<1, 1000> > >'

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
		Event oscilloscope_mode_event;

		using CAENInterfaceState
			= BlockingTotalTimeEvent<
				std::function<bool(void)>,
				std::chrono::milliseconds
			>;

		using CAENInterfaceState_ptr = std::shared_ptr<CAENInterfaceState>;

		CAENInterfaceState_ptr main_loop_state;

		CAENInterfaceState_ptr standby_state;
		CAENInterfaceState_ptr attemptConnection_state;
		CAENInterfaceState_ptr oscilloscopeMode_state;
		CAENInterfaceState_ptr statisticsMode_state;
		CAENInterfaceState_ptr runMode_state;
		CAENInterfaceState_ptr disconnected_state;
		CAENInterfaceState_ptr closing_state;

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

			standby_state = std::make_shared<CAENInterfaceState>(
				std::chrono::milliseconds(100),
				std::bind(&CAENDigitizerInterface::standby, this)
			);

			attemptConnection_state = std::make_shared<CAENInterfaceState>(
				std::chrono::milliseconds(1),
				std::bind(&CAENDigitizerInterface::attempt_connection, this)
			);

			oscilloscopeMode_state = std::make_shared<CAENInterfaceState>(
				std::chrono::milliseconds(100),
				std::bind(&CAENDigitizerInterface::oscilloscope, this)
			);

			statisticsMode_state = std::make_shared<CAENInterfaceState>(
				std::chrono::milliseconds(1),
				std::bind(&CAENDigitizerInterface::statistics_mode, this)
			);

			runMode_state = std::make_shared<CAENInterfaceState>(
				std::chrono::milliseconds(1),
				std::bind(&CAENDigitizerInterface::run_mode, this)
			);

			disconnected_state = std::make_shared<CAENInterfaceState>(
				std::chrono::milliseconds(1),
				std::bind(&CAENDigitizerInterface::disconnected_mode, this)
			);

			// auto g =
			closing_state = std::make_shared<CAENInterfaceState>(
				std::chrono::milliseconds(1),
				std::bind(&CAENDigitizerInterface::closing_mode, this)
			);

			main_loop_state = standby_state;

			// Actual loop!
			while(main_loop_state->operator()());

			// Make sure the resources are freed.
			disconnect(port);

		}

private:
		void processing() {
			CAEN_ptr& port = state_of_everything.Port;

			static auto extract_for_gui = [&]() {
				if(port->Data.DataSize <= 0) {
					return;
				}

				// For the GUI
				std::default_random_engine generator;
				std::uniform_int_distribution<uint32_t>
					distribution(0,port->Data.NumEvents);
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
				if(port->Data.DataSize <= 0) {
					return;
				}

				double frequency = 0.0;
				spdlog::info("There are {0} events", port->Data.NumEvents);
				for(uint32_t i = 0; i < port->Data.NumEvents; i++) {

					// Extract event i
					Event event = extract_event(port, i);

					// Process events!
					//frequency += extract_frequency(event);

					// Free event i, we no longer need the memory
					free_event(port, event);
				}

				frequency /= port->Data.NumEvents;
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
		
		// Local error checking. Checks if there was an error and prints it
		// using spdlog
		bool lec() {
			return check_error(state_of_everything.Port,
				[](const std::string& cmd) {
					spdlog::error(cmd);
			});
		}

		void switch_state(const CAENInterfaceStates& newState) {
			state_of_everything.CurrentState = newState;
			switch(state_of_everything.CurrentState) {
				case CAENInterfaceStates::Standby:
					main_loop_state = standby_state;
				break;

				case CAENInterfaceStates::AttemptConnection:
					main_loop_state = attemptConnection_state;
				break;

				case CAENInterfaceStates::OscilloscopeMode:
					main_loop_state = oscilloscopeMode_state;
				break;

				case CAENInterfaceStates::StatisticsMode:
					main_loop_state = statisticsMode_state;
				break;

				case CAENInterfaceStates::RunMode:
					main_loop_state = runMode_state;
				break;

				case CAENInterfaceStates::Disconnected:
					main_loop_state = disconnected_state;
				break;

				case CAENInterfaceStates::Closing:
					main_loop_state = closing_state;
				break;

				case CAENInterfaceStates::NullState:
				default:
					// do nothing other than set to standby state
					main_loop_state = standby_state;
			}
		}

		// Envelopes the logic that listens to an external queue
		// that can the data inside this thread.
		void change_state() {
			// GUI -> CAEN
			CAENQueue& guiQueueOut = std::get<CAENQueue&>(_queues);
			CAENQueueType task;

			// If the queue does not return a valid function, this call will
			// do nothing and should return true always.
			// The tasks are essentially any GUI driven modification, example
			// setting the PID setpoints or constants
			// or an user driven reset
			if(guiQueueOut.try_dequeue(task)) {
				if(!task(state_of_everything)) {
					spdlog::warn("Something went wrong with a command!");
				} else {
					switch_state(state_of_everything.CurrentState);
				}
			}

		}

		// Does nothing other than wait 100ms to avoid clogging PC resources.
		bool standby() {
			change_state();

			return true;
		}

		// Attempts a connection to the CAEN digitizer, setups the channels
		// and starts acquisition
		bool attempt_connection() {
			CAEN_ptr& port = state_of_everything.Port;
			connect_usb(port, state_of_everything.PortNum);

			if(port) {
				spdlog::info("Connected to CAEN Digitizer!");

				reset(port);
				setup(port, {state_of_everything.Config});
				allocate_event(port, oscilloscope_mode_event);
				enable_acquisition(port);

				auto failed = lec();

				if(failed) {
					spdlog::warn("Failed to setup CAEN.");
					switch_state(CAENInterfaceStates::Standby);
				} else {
					spdlog::info("CAEN Setup complete!");
					switch_state(CAENInterfaceStates::OscilloscopeMode);
				}

			} else {
				spdlog::warn("Failed to connect to CAEN Digitizer "
					"with USB port:");
				switch_state(CAENInterfaceStates::Standby);
			}

			change_state();

			return true;
		}

		// While in this state it shares the data with the GUI but
		// no actual file saving is happening. It essentially serves
		// as a mode in where the user can see what is happening.
		// Similar to an oscilloscope
		bool oscilloscope() {

			CAEN_ptr& port = state_of_everything.Port;

			auto events = get_events_in_buffer(port);
			_plotSender(IndicatorNames::CAENBUFFEREVENTS, events);

			retrieve_data(port);

			if(port->Data.DataSize > 0 && port->Data.NumEvents > 0) {

				Event evt = extract_event(port, 0);

				auto buf = evt.Data->DataChannel[0];
				auto size = evt.Data->ChSize[0];

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

				if(port->Data.NumEvents >= 2) {
					Event adj_evt = extract_event(port, 1);

					auto t0 = evt.Info.TriggerTimeTag;
					auto t1 = adj_evt.Info.TriggerTimeTag;

					spdlog::info("Time difference between events: {0}", t1 - t0);
				}

				// Clear data to have the most up-to-date data
				free_event(port, evt);
				clear_data(port);
			}

			lec();
			change_state();
			return true;
		}

		// Does the SiPM pulse processing but no file saving
		// is done. Serves more for the user to know
		// how things are changing.
		bool statistics_mode() {
			processing();
			change_state();
			return true;
		}

		bool run_mode() {
			processing();
			save_data();
			change_state();
			return true;
		}

		bool disconnected_mode() {
			spdlog::warn("Manually losing connection to the "
							"CAEN digitizer.");

			free_event(state_of_everything.Port, oscilloscope_mode_event);
			disconnect(state_of_everything.Port);

			switch_state(CAENInterfaceStates::Standby);
			return true;
		}

		bool closing_mode() {
			spdlog::info("Going to close the CAEN thread.");
			free_event(state_of_everything.Port, oscilloscope_mode_event);
			return false;
		}
	};
} // namespace SBCQueens