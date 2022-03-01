#pragma once

// std includes
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <numeric>
#include <string>
#include <thread>
#include <chrono>
#include <random>
#include <iostream>

// 3rd party includes
#include <readerwriterqueue.h>
#include <spdlog/spdlog.h>

// my includes
#include "file_helpers.h"
#include "caen_helper.h"
#include "implot_helpers.h"
#include "include/caen_helper.h"
#include "include/timing_events.h"
#include "indicators.h"
#include "spdlog/spdlog.h"
#include "timing_events.h"

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
		std::string RunName =  "";
		std::string SiPMParameters = "";

		CAENDigitizerModel Model;
		CAENGlobalConfig GlobalConfig;
		std::vector<CAENGroupConfig> GroupConfigs;

		int PortNum = 0;

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

		DataFile<CAENEvent> _pulseFile;

		IndicatorSender<IndicatorNames> _plotSender;

		CAEN Port = nullptr;

		//tmp stuff
		uint16_t* data;
		size_t length;
		double* x_values, *y_values;
		CAENEvent osc_event, adj_osc_event;
		// 1024 because the caen can only hold 1024 no matter what model (so far)
		CAENEvent processing_evts[1024];

		// As long as we make the Func template argument a std::fuction
		// we can then use pointer magic to initialize them
		using CAENInterfaceState
			= BlockingTotalTimeEvent<
				std::function<bool(void)>,
				std::chrono::milliseconds
			>;

		// We need to turn the class BlockingTotalTimeEvent into a pointer
		// because it does not have an empty constructor and for a good reason
		// if its empty, bugs and crashes would happen all the time.
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
			_plotSender(std::get<SiPMsPlotQueue&>(_queues)) {
			// This is possible because std::function can be assigned
			// to whatever std::bind returns
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
		}

		// No copying
		CAENDigitizerInterface(const CAENDigitizerInterface&) = delete;

		~CAENDigitizerInterface() {}

		void operator()() {

			spdlog::info("Initializing CAEN thread");

			main_loop_state = standby_state;

			// Actual loop!
			while(main_loop_state->operator()());

		}

private:

		double extract_frequency(CAENEvent& evt) {
			auto buf = evt->Data->DataChannel[0];
			auto size = evt->Data->ChSize[0];

			double counter = 0;
			auto offset =
				Port->GroupConfigs[0].TriggerThreshold;
			for(uint32_t i = 1; i < size; i++) {
				if((buf[i] > offset) && (buf[i-1] < offset)) {
					counter++;
				}
			}

			return Port->GetSampleRate()*counter /
				(Port->GlobalConfig.RecordLength);
		}
		
		// Local error checking. Checks if there was an error and prints it
		// using spdlog
		bool lec() {
			return check_error(Port,
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
		bool change_state() {
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
					return true;
				}
			}

			return false;

		}

		// Does nothing other than wait 100ms to avoid clogging PC resources.
		bool standby() {
			change_state();

			return true;
		}

		// Attempts a connection to the CAEN digitizer, setups the channels
		// and starts acquisition
		bool attempt_connection() {
			auto err = connect_usb(Port,
				state_of_everything.Model,
				state_of_everything.PortNum);

			// If port resource was not created, it equals a failure!
			if(!Port) {
				// Print what was the error
				check_error(err, [](const std::string& cmd) {
					spdlog::error(cmd);
				});

				// We disconnect because this will free resources (in case
				// they were created)
				disconnect(Port);
				switch_state(CAENInterfaceStates::Standby);
				return true;
			}

			spdlog::info("Connected to CAEN Digitizer!");

			reset(Port);
			setup(Port,
				state_of_everything.GlobalConfig,
				state_of_everything.GroupConfigs);

			enable_acquisition(Port);

			// Allocate memory for events
			osc_event = std::make_shared<caenEvent>(Port->Handle);

			for(CAENEvent& evt : processing_evts) {
				evt = std::make_shared<caenEvent>(Port->Handle);
			}

			auto failed = lec();

			if(failed) {
				spdlog::warn("Failed to setup CAEN");
				err = disconnect(Port);
				check_error(err, [](const std::string& cmd) {
					spdlog::error(cmd);
				});

				switch_state(CAENInterfaceStates::Standby);

			} else {
				spdlog::info("CAEN Setup complete!");
				switch_state(CAENInterfaceStates::OscilloscopeMode);
			}

			change_state();

			return true;
		}

		// While in this state it shares the data with the GUI but
		// no actual file saving is happening. It essentially serves
		// as a mode in where the user can see what is happening.
		// Similar to an oscilloscope
		bool oscilloscope() {

			auto events = get_events_in_buffer(Port);
			_plotSender(IndicatorNames::CAENBUFFEREVENTS, events);

			retrieve_data(Port);

			if(Port->Data.DataSize > 0 && Port->Data.NumEvents > 0) {

				// spdlog::info("Total size buffer: {0}",  Port->Data.TotalSizeBuffer);
				// spdlog::info("Data size: {0}", Port->Data.DataSize);
				// spdlog::info("Num events: {0}", Port->Data.NumEvents);

				extract_event(Port, 0, osc_event);
				// spdlog::info("Event size: {0}", osc_event->Info.EventSize);
				// spdlog::info("Event counter: {0}", osc_event->Info.EventCounter);
				// spdlog::info("Trigger Time Tag: {0}", osc_event->Info.TriggerTimeTag);

				process_data_for_gui();

				// if(port->Data.NumEvents >= 2) {
				// 	extract_event(port, 1, adj_osc_event);

				// 	auto t0 = osc_event->Info.TriggerTimeTag;
				// 	auto t1 = adj_osc_event->Info.TriggerTimeTag;

				// 	spdlog::info("Time difference between events: {0}", t1 - t0);
				// }


			}
			// Clear events in buffer
			clear_data(Port);

			lec();
			change_state();
			return true;
		}

		// Does the SiPM pulse processing but no file saving
		// is done. Serves more for the user to know
		// how things are changing.
		bool statistics_mode() {

			static auto process_events = [&]() {
				bool isData = retrieve_data_until_n_events(Port, 
					+Port->GlobalConfig.MaxEventsPerRead);

				// While all of this is happening, the digitizer is taking data
				if(!isData) {
					return;
				}

				//double frequency = 0.0;
				for(uint32_t i = 0; i < Port->Data.NumEvents; i++) {

					// Extract event i
					extract_event(Port, i, processing_evts[i]);
				}

			};

			static auto extract_for_gui_nb = make_total_timed_event(
				std::chrono::milliseconds(200),
				std::bind(&CAENDigitizerInterface::rdm_extract_for_gui, this)
			);

			static auto checkerror = make_total_timed_event(
				std::chrono::seconds(1),
				std::bind(&CAENDigitizerInterface::lec, this)
			);


			process_events();
			checkerror();
			extract_for_gui_nb();
			//extract_for_gui_nb();
			change_state();
			return true;
		}

		bool run_mode() {
			static bool isFileOpen = false;
			static auto extract_for_gui_nb = make_total_timed_event(
				std::chrono::milliseconds(200),
				std::bind(&CAENDigitizerInterface::rdm_extract_for_gui, this)
			);
			static auto process_events = [&]() {
				bool isData = retrieve_data_until_n_events(Port, 
					+Port->GlobalConfig.MaxEventsPerRead);

				// While all of this is happening, the digitizer is taking data
				if(!isData) {
					return;
				}

				//double frequency = 0.0;
				for(uint32_t i = 0; i < Port->Data.NumEvents; i++) {

					// Extract event i
					extract_event(Port, i, processing_evts[i]);

					if(_pulseFile) {
						// Copy event to the file buffer
						_pulseFile->Add(processing_evts[i]);
					}
				}
				spdlog::info("Saving");
				_pulseFile->flush();

			};

			if(isFileOpen) {
				// spdlog::info("Saving SIPM data");
				save(_pulseFile, sbc_save_func, Port);
			} else {
				
				// Get current date and time, e.g. 202201051103
				std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
				std::time_t now_t = std::chrono::system_clock::to_time_t(now);
				char filename[16];
    			std::strftime(filename, sizeof(filename), "%Y%m%d%H%M", 
    				std::localtime(&now_t));

				open(_pulseFile,
					state_of_everything.RunDir
					+ "/" + state_of_everything.RunName
					+ "/" + filename + ".bin",
					// state_of_everything.SiPMParameters

					// sbc_init_file is a function that saves the header
					// of the sbc data format as a function of record length
					// and number of channels
					sbc_init_file,
					Port);

				isFileOpen = _pulseFile > 0;
			}
			process_events();
			extract_for_gui_nb();
			if(change_state()) {
				isFileOpen = false;
				close(_pulseFile);
			}
			return true;
		}

		bool disconnected_mode() {
			spdlog::warn("Manually losing connection to the "
							"CAEN digitizer.");

			for(CAENEvent& evt : processing_evts) {
				evt.reset();
			}

			osc_event.reset();
			adj_osc_event.reset();

			auto err = disconnect(Port);
			check_error(err, [](const std::string& cmd) {
				spdlog::error(cmd);
			});

			switch_state(CAENInterfaceStates::Standby);
			return true;
		}

		bool closing_mode() {
			spdlog::info("Going to close the CAEN thread.");
			for(CAENEvent& evt : processing_evts) {
				evt.reset();
			}

			osc_event.reset();
			adj_osc_event.reset();

			auto err = disconnect(Port);
			check_error(err, [](const std::string& cmd) {
				spdlog::error(cmd);
			});

			return false;
		}

		void rdm_extract_for_gui() {
			if(Port->Data.DataSize <= 0) {
				return;
			}

			// For the GUI
			std::default_random_engine generator;
			std::uniform_int_distribution<uint32_t>
				distribution(0, Port->Data.NumEvents);
			uint32_t rdm_num = distribution(generator);

			extract_event(Port, rdm_num, osc_event);

			process_data_for_gui();
		};

		void process_data_for_gui() {
			// int j = 0;

			//TODO(Zhiheng): change anything you need here, too...
			// for(auto ch : Port->GroupConfigs) {
			for (int j=0; j<3; j++) {
				// auto& ch_num = ch.second.Number;
				auto buf = osc_event->Data->DataChannel[j];
				auto size = osc_event->Data->ChSize[j];

				//spdlog::info("Event size size: {0}", size);
				if(size <= 0) {
					continue;
				}

				x_values = new double[size];
				y_values = new double[size];

				for(uint32_t i = 0; i < size; i++) {
					x_values[i] = i*(1.0/Port->GetSampleRate())*(1e9);
					y_values[i] = static_cast<double>(buf[i]);
				}

				IndicatorNames plotToSend;
				switch(j) {

					case 1:
						plotToSend = IndicatorNames::SiPM_Plot_ONE;
					break;

					case 2:
						plotToSend = IndicatorNames::SiPM_Plot_TWO;
					break;

					case 3:
						plotToSend = IndicatorNames::SiPM_Plot_THREE;
					break;

					case 0:
					default:
						plotToSend = IndicatorNames::SiPM_Plot_ZERO;
				}

				_plotSender(plotToSend,
					x_values,
					y_values,
					size);

				delete x_values;
				delete y_values;

				// j++;
			}
		}
	};
} // namespace SBCQueens
