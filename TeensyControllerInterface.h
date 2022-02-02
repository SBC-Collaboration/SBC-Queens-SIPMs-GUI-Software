#pragma once

/*

	This class manages the logic behind the communication with the
	Teensy microcontroller.

	It holds the serial port. Includes crc32 checksum, checks for checksum,
	translates the teensy responses, and sends them to the other threads
	for further analysis or displaying.

	There are a bunch of structs and functions here that deal with the low level
	stuff required to communicate with the teensy as efficiently as possible.

*/

// STD includes
#include "include/timing_events.h"
#include <cstdint>
#include <inttypes.h>
#include <string>
#include <chrono>
#include <thread>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <optional>
#include <future>
#include <map>

// We need this because chrono has long names...
namespace chrono = std::chrono; 

// Third party includes
#include <readerwriterqueue.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <toml.hpp>
using json = nlohmann::json;
// #include <boost/sml.hpp>
// namespace sml = boost::sml;

// My includes
#include "serial_helper.h"
#include "imgui_helpers.h"
#include "implot_helpers.h"
#include "file_helpers.h"
#include "timing_events.h"
#include "indicators.h"

namespace SBCQueens {

	enum class TeensyControllerStates {
		NullState = 0,
		Standby,
		AttemptConnection,
		Connected,
		Disconnected,
		Closing
	};

	enum class PIDState {
		Standby,
		Running
	};

	struct PIDConfig {
		float SetPoint = 0.0;
		float Kp = 0.0;
		float Ti = 0.0;
		float Td = 0.0;
	};

	// Sensors structs
	struct PIDValues {
		double Current;
		double Temperature;
	};

	struct PIDs {
		double time;
		PIDValues PID1;
		PIDValues PID2;
	};

	// The BMEs return their values as registers
	// as the calculations required to turn them into
	// temp, hum  or pressure are expensive for a MC.
	struct BMEValues_t {
		double Temperature;
		double Pressure;
		double Humidity;
	};

	struct BMEs {
		double time;
		BMEValues_t LocalBME;
		BMEValues_t BoxBME;
	};

	struct BME_COMPENSATION_REGISTERS {
		uint16_t dig_T1;
		int16_t dig_T2;
		int16_t dig_T3;

		uint16_t dig_P1;
		int16_t dig_P2;
		int16_t dig_P3;
		int16_t dig_P4;
		int16_t dig_P5;
		int16_t dig_P6;
		int16_t dig_P7;
		int16_t dig_P8;
		int16_t dig_P9;

		uint8_t dig_H1;
		int16_t dig_H2;
		uint8_t dig_H3;
		int16_t dig_H4;
		int16_t dig_H5;
		int8_t dig_H6;
	};

	enum class BME_TYPE {LOCAL, BOX};
	// These are the constant compensation or calibration parameters for each BME
	const BME_COMPENSATION_REGISTERS LOCAL_COMPENSATION {
		// Temps
		28414u, 26857, 50, 
		// Pressure
		38146u, -10599, 3024, 7608, -223, -7, 9900, -10230, 4285, 
		// Hum
		75u, 380, 0u, 277, 50, 30
	};

	const BME_COMPENSATION_REGISTERS BOX_COMPENSATION {
		// Temps
		28155u, 26962, 50, 
		// Pressure
		37389u, -10497, 3024, 9736, -150, -7, 9900, -10230, 4285, 
		// Humidity
		75u, 378, 0u, 281, 50, 30
	};

	const std::unordered_map<BME_TYPE, BME_COMPENSATION_REGISTERS> bme_comp_map = {
		{BME_TYPE::LOCAL, LOCAL_COMPENSATION},
		{BME_TYPE::BOX, BOX_COMPENSATION}
	};
	
	// BME Compensation functions
	// Taken directly from the datasheet modified by me to optimize for
	// running time and different BMEs
	inline double t_fine_to_temp(const int32_t& t_fine);

	template<BME_TYPE T>
	int32_t BME_calculate_t_fine(const int32_t& adc_T);

	template<BME_TYPE T>
	double BME280_compensate_P_double(const int32_t& adc_P, const int32_t& t_fine);

	template<BME_TYPE T>
	double BME280_compensate_H_double(const int32_t& adc_H, const int32_t& t_fine);

	// These functions are required for json to convert to our types.
	// Turns PIDs type to Json. 
    void to_json(json& j, const PIDs& p);
    // Turns a JSON to PIDs
    void from_json(const json& j, PIDs& p);
    // Turns BMEs type to Json. 
    void to_json(json& j, const BMEs& p);
    // Turns a JSON to BMEs
    void from_json(const json& j, BMEs& p);
	// end Sensors structs

	// It holds everything the outside world can modify or use.
	// So far, I do not like teensy_serial is here.
	struct TeensyControllerState {

		std::string RunDir = "";
		std::string RunName = "";
		
		std::string Port = "COM4";
		serial_ptr teensy_serial;

		TeensyControllerStates CurrentState 
			= TeensyControllerStates::NullState;

		// Relay stuff
		bool PIDRelayState = false;
		bool GeneralRelayState = false;

		// PID Stuff
		PIDState PIDOneState;
		PIDConfig PIDOneCurrentValues;
		PIDConfig PIDOneTempValues;

		PIDState PIDTwoState;
		PIDConfig PIDTwoCurrentValues;
		PIDConfig PIDTwoTempValues;
	};

	using TeensyQueueInType 
		= std::function < bool(TeensyControllerState&) >;

	// single consumer, single sender queue for Tasks of the type
	// bool(TeensyControllerState&) A.K.A TeensyQueueInType
	using TeensyInQueue 
		= moodycamel::ReaderWriterQueue< TeensyQueueInType >;


	enum class TeensyCommands {
		CheckError,
		GetError,
		Reset,

		StartPIDOne,
		SetPIDOneTempSetpoint,
		SetPIDOneTempKp,
		SetPIDOneTempTi,
		SetPIDOneTempTd,

		SetPIDOneCurrSetpoint,
		SetPIDOneCurrKp,
		SetPIDOneCurrTi,
		SetPIDOneCurrTd,
		StopPIDOne,

		StartPIDTwo,
		SetPIDTwoTempSetpoint,
		SetPIDTwoTempKp,
		SetPIDTwoTempTi,
		SetPIDTwoTempTd,

		SetPIDTwoCurrSetpoint,
		SetPIDTwoCurrKp,
		SetPIDTwoCurrTi,
		SetPIDTwoCurrTd,
		StopPIDTwo,


		SetPIDRelay,
		SetGeneralRelay,

		GetPIDs,
		GetBMEs

	};

	// This map holds all the commands that the Teensy accepts, as str,
	// and maps them to an enum for easy access.
	const std::unordered_map<TeensyCommands, std::string> cTeensyCommands = {
		/// General system commands
		{TeensyCommands::CheckError, 			"CHECKERR"},
		{TeensyCommands::Reset, 				"RESET"},
		/// !General system commands
		////
		/// Hardware specific commands
		{TeensyCommands::StartPIDOne, 			"START_PID1"},
		{TeensyCommands::SetPIDOneTempSetpoint, "SET_TEMP1"},
		{TeensyCommands::SetPIDOneTempKp, 		"SET_TKP_PID1"},
		{TeensyCommands::SetPIDOneTempTi, 		"SET_TTi_PID1"},
		{TeensyCommands::SetPIDOneTempTd, 		"SET_TTd_PID1"},
		{TeensyCommands::SetPIDOneCurrSetpoint, "SET_CURRENT1"},
		{TeensyCommands::SetPIDOneCurrKp, 		"SET_AKP_PID1"},
		{TeensyCommands::SetPIDOneCurrTi, 		"SET_ATi_PID1"},
		{TeensyCommands::SetPIDOneCurrTd, 		"SET_ATd_PID1"},
		{TeensyCommands::StopPIDOne, 			"STOP_PID1"},

		{TeensyCommands::StartPIDTwo, 			"START_PID2"},
		{TeensyCommands::SetPIDTwoTempSetpoint, "SET_TEMP2"},
		{TeensyCommands::SetPIDTwoTempKp, 		"SET_TKP_PID2"},
		{TeensyCommands::SetPIDTwoTempTi, 		"SET_TTi_PID2"},
		{TeensyCommands::SetPIDTwoTempTd, 		"SET_TTd_PID2"},
		{TeensyCommands::SetPIDTwoCurrSetpoint, "SET_CURRENT2"},
		{TeensyCommands::SetPIDTwoCurrKp, 		"SET_AKP_PID2"},
		{TeensyCommands::SetPIDTwoCurrTi, 		"SET_ATi_PID2"},
		{TeensyCommands::SetPIDTwoCurrTd, 		"SET_ATd_PID2"},
		{TeensyCommands::StopPIDTwo, 			"STOP_PID2"},

		{TeensyCommands::SetPIDRelay, 			"SET_PID_RELAY"},
		{TeensyCommands::SetGeneralRelay, 		"SET_GEN_RELAY"},

		//// Getters
		{TeensyCommands::GetError, 				"GETERR"},
		{TeensyCommands::GetPIDs, 				"GET_PIDS"},
		{TeensyCommands::GetBMEs, 				"GET_BMES"},
		//// !Getters
		/// !Hardware specific commands
	};

	// Sends an specific command from the available commands
	bool send_teensy_cmd(const TeensyCommands& cmd, TeensyControllerState& tcs);

	template<typename... Queues>
	class TeensyControllerInterface {

	private:

		std::tuple<Queues&...> _queues;
		std::map<std::string, std::string> _crc_cmds;

		TeensyControllerState state_of_everything;
		IndicatorSender<IndicatorNames> _plotSender;

		double _init_time;

		DataFile<PIDs> _PIDsFile;
		DataFile<BMEs> _BMEsFile;

	public:

		explicit TeensyControllerInterface(Queues&... queues) 
			: _queues(forward_as_tuple(queues...)),
			_plotSender(std::get<SiPMsPlotQueue&>(_queues)) { }

		// No copying
		TeensyControllerInterface(const TeensyControllerInterface&) = delete;

		~TeensyControllerInterface() {}

		// Loop goes like this:
		// Initializes -> waits for action from GUI to connect 
		//	-> Update (every dt or at f) (retrieves info or updates teensy) ->
		//  -> Update until disconnect, close, or error.
		void operator()() {
			
			serial_ptr& teensy_port = state_of_everything.teensy_serial;

			auto connect_bt = make_blocking_total_timed_event(
				std::chrono::milliseconds(5000), connect
			);

			spdlog::info("Initializing teensy thread");

			// GUI -> Teensy
			TeensyInQueue& guiQueueOut = std::get<TeensyInQueue&>(_queues);
			auto guiQueueFunc = [&guiQueueOut]() -> SBCQueens::TeensyQueueInType {

				SBCQueens::TeensyQueueInType new_task;
				bool success = guiQueueOut.try_dequeue(new_task);

				if(success) {
					return new_task;
				} else { 
					return [](SBCQueens::TeensyControllerState&) { return true; }; 
				}
			};


			// Main loop lambda
			auto main_loop = [&]() -> bool {
				TeensyQueueInType task = guiQueueFunc();

				// If the queue does not return a valid function, this call will
				// do nothing and should return true always.
				// The tasks are essentially any GUI driven modifcation, example
				// setting the PID setpoints or constants
				// or an user driven reset
				if(!task(state_of_everything)) {
					spdlog::warn("Something went wrong with a command!");
				}

				// End Communication with the GUI

				// This will turn into an SML soon*
				switch(state_of_everything.CurrentState) {

					case TeensyControllerStates::Standby:
						// do nothing
						// only way to get out of this state is by the GUI
						// starting a connection
						std::this_thread::sleep_for(
							std::chrono::milliseconds(100));
						break;

					case TeensyControllerStates::Disconnected:
						spdlog::warn("Manually losing connection to "
							"Teensy with port {}", state_of_everything.Port);

						disconnect(teensy_port);

						// Move to standby
						state_of_everything.CurrentState 
							= TeensyControllerStates::Standby;
						break;

					case TeensyControllerStates::Connected:
						// The real bread and butter of the code!
						update(state_of_everything);
						break;

					case TeensyControllerStates::AttemptConnection:
						
						connect_bt(teensy_port, state_of_everything.Port);

						if(teensy_port) {
							if(teensy_port->isOpen()) {

								// t0 = time when the communication was 100%
								_init_time = get_current_time_epoch();

								// Open files to start saving!
								open(_PIDsFile,
									state_of_everything.RunDir
									+ "/" + state_of_everything.RunName
									+ "/PIDs.txt");
						 		bool s = _PIDsFile > 0;

								open(_BMEsFile,
									state_of_everything.RunDir
									+ "/" + state_of_everything.RunName
									+ "/BMEs.txt");
								s &= _BMEsFile > 0;

								if(!s) {
									spdlog::error("Failed to open files.");
									state_of_everything.CurrentState =
										TeensyControllerStates::Standby;
								} else {
									spdlog::info(
										"Connected to Teensy with port {}",
									state_of_everything.Port);

									send_initial_config();

									state_of_everything.CurrentState =
										TeensyControllerStates::Connected;
								}

							} else {
								spdlog::error("Failed to connect to port {}", 
									state_of_everything.Port);

								state_of_everything.CurrentState 
									= TeensyControllerStates::Standby;
							}
						} else {
							// No need for a message
							state_of_everything.CurrentState 
								= TeensyControllerStates::Standby;
						}
						
						break;

					case TeensyControllerStates::Closing:
						spdlog::info("Going to close the Teensy thread.");
						return false;

					case TeensyControllerStates::NullState:
					default:
						// do nothing other than set to standby state
						state_of_everything.CurrentState = TeensyControllerStates::Standby;

				}

				// Only one case, the Closing state will return false;
				return true;
				// * = As soon as I feel learning that library is worth it.
			};
			
			// This will call rlf_block_time every 1 ms 
			// microseconds to have better resolution or less error perhaps
			auto main_loop_block_time = make_blocking_total_timed_event(
				std::chrono::microseconds(1000), main_loop
			);
			
			// Actual loop!
			while(main_loop_block_time());

		} // here port should get deleted

private:
		void send_initial_config() {

			auto send_ts_cmd_b = make_blocking_total_timed_event(
				std::chrono::milliseconds(10),
					send_teensy_cmd
				);

			send_ts_cmd_b(TeensyCommands::SetPIDOneTempSetpoint,
				state_of_everything);
			send_ts_cmd_b(TeensyCommands::SetPIDOneTempKp,
				state_of_everything);
			send_ts_cmd_b(TeensyCommands::SetPIDOneTempTd,
				state_of_everything);
			send_ts_cmd_b(TeensyCommands::SetPIDOneTempTi,
				state_of_everything);

			send_ts_cmd_b(TeensyCommands::SetPIDOneCurrSetpoint,
				state_of_everything);
			send_ts_cmd_b(TeensyCommands::SetPIDOneCurrKp,
				state_of_everything);
			send_ts_cmd_b(TeensyCommands::SetPIDOneCurrTd,
				state_of_everything);
			send_ts_cmd_b(TeensyCommands::SetPIDOneCurrTi,
				state_of_everything);

			send_ts_cmd_b(TeensyCommands::SetPIDTwoTempSetpoint,
				state_of_everything);
			send_ts_cmd_b(TeensyCommands::SetPIDTwoTempKp,
				state_of_everything);
			send_ts_cmd_b(TeensyCommands::SetPIDTwoTempTd,
				state_of_everything);
			send_ts_cmd_b(TeensyCommands::SetPIDTwoTempTi,
				state_of_everything);

			send_ts_cmd_b(TeensyCommands::SetPIDTwoCurrSetpoint,
				state_of_everything);
			send_ts_cmd_b(TeensyCommands::SetPIDTwoCurrKp,
				state_of_everything);
			send_ts_cmd_b(TeensyCommands::SetPIDTwoCurrTd,
				state_of_everything);
			send_ts_cmd_b(TeensyCommands::SetPIDTwoCurrTi,
				state_of_everything);

			// Flush so there is nothing in the buffer
			// making everything annoying
			flush(state_of_everything.teensy_serial);
		}

		void retrieve_pids(TeensyControllerState& teensyState) {

			if(!send_teensy_cmd(TeensyCommands::GetPIDs, teensyState)) {
				spdlog::warn("Failed to send retrieve_pids to Teensy.");
				flush(state_of_everything.teensy_serial);
				return;
			}
				
			auto msg_opt = retrieve_msg<std::string>(teensyState.teensy_serial);

			if(!msg_opt.has_value()) {
				spdlog::warn("Failed to retrieve latest Teensy PID values.");
				flush(state_of_everything.teensy_serial);
				return;
			}
			
			// if json_pck is empty or has the incorrect format this will fail
			json json_parse = json::parse(msg_opt.value(), nullptr, false);
			if(json_parse.is_discarded()) {
				spdlog::warn("Invalid json string. "
							"Message received from Teensy: {0}", msg_opt.value());
				flush(state_of_everything.teensy_serial);
				return;
			}

			// No way to avoid this try-catch block. get<>() does not have a
			// version that does not throw...
			try {

				auto pids = json_parse.get<PIDs>();
				
				// Time since communication with the Teensy
				double dt = (pids.time - _init_time) / 1000.0;

				// Send them to GUI to draw them
				_plotSender(IndicatorNames::PID1_Temps,
					dt, pids.PID1.Temperature);
				_plotSender(IndicatorNames::PID1_Currs,
					dt, pids.PID1.Current);
				_plotSender(IndicatorNames::PID2_Temps,
					dt, pids.PID2.Temperature);
				_plotSender(IndicatorNames::PID2_Currs,
					dt, pids.PID2.Current);

				_plotSender(IndicatorNames::LATEST_PID1_TEMP,
					pids.PID1.Temperature);
				_plotSender(IndicatorNames::LATEST_PID1_CURR,
					pids.PID1.Current);
				_plotSender(IndicatorNames::LATEST_PID2_TEMP,
					pids.PID2.Temperature);
				_plotSender(IndicatorNames::LATEST_PID2_CURR,
					pids.PID2.Current);

				_PIDsFile->Add(pids);

			} catch (... ) {
				spdlog::warn("Failed to parse lastest Teensy PID values. "
							"Message received from Teensy: {0}", msg_opt.value());
			}

			// then save them to file, maybe do it
			// in batches? at the end of the run if
			// RAM allows it?
			// The save format has to be the same as
			// SBC/PICO uses
		}

		void retrieve_bmes(TeensyControllerState& teensyState) {
			// Same as above but for now, not implemented
			if(!send_teensy_cmd(TeensyCommands::GetBMEs, teensyState)) {
				spdlog::warn("Failed to send retrieve_bmes to Teensy.");
				flush(state_of_everything.teensy_serial);
				return;
			}
				
			auto msg_opt = retrieve_msg<std::string>(teensyState.teensy_serial);

			if(!msg_opt.has_value()) { 
				spdlog::warn("Failed to retrieve latest Teensy BME values.");
				flush(state_of_everything.teensy_serial);
				return; 
			}

			// if json_pck is empty this will fail or has the incorrect format
			// this will fail
			json json_parse = json::parse(msg_opt.value(), nullptr, false);
			if(json_parse.is_discarded()) {
				spdlog::warn("Invalid json string. "
					"Message received from Teensy: {0}", msg_opt.value());
				return;
			}

			try {

				auto bmes = json_parse.get<BMEs>(); 

				// Time since communication with the Teensy
				double dt = (bmes.time - _init_time) / 1000.0;

				_plotSender(IndicatorNames::LOCAL_BME_Temps,
					dt, bmes.LocalBME.Temperature);
				_plotSender(IndicatorNames::LOCAL_BME_Pressure,
					dt, bmes.LocalBME.Pressure);
				_plotSender(IndicatorNames::LOCAL_BME_Humidity,
					dt, bmes.LocalBME.Humidity);

				_plotSender(IndicatorNames::BOX_BME_Temps,
					dt, bmes.BoxBME.Temperature);
				_plotSender(IndicatorNames::BOX_BME_Pressure,
					dt, bmes.BoxBME.Pressure);
				_plotSender(IndicatorNames::BOX_BME_Humidity,
					dt, bmes.BoxBME.Humidity);

				_plotSender(IndicatorNames::LATEST_BOX_BME_HUM,
					bmes.BoxBME.Humidity);
				_plotSender(IndicatorNames::LATEST_BOX_BME_TEMP,
					bmes.BoxBME.Temperature);

				_BMEsFile->Add(bmes);

			} catch (... ) {
				spdlog::warn("Failed to parse lastest Teensy PID values. "
							"Message received from Teensy: {0}", msg_opt.value());
			}

			// Send them to GUI to draw them
			// then save them to file, maybe do it
			// in batches? at the end of the run if
			// RAM allows it?
			// The save format has to be the same as
			// SBC/PICO uses

		}

		// It continuosly polls the Teensy for the latest data and saves it
		// to the file and updates the GUI graphs
		void update(TeensyControllerState& teensyState) {

			// We do not need this function to be out of this scope
			// and make it static to call it once
			static auto retrieve_pids_nb = make_total_timed_event(
				std::chrono::milliseconds(100),  
				// Lambda hacking to allow the class function to be pass to 
				// make_total_timed_event. Is there any other way?
				[&](TeensyControllerState& teensyState) {
					retrieve_pids(teensyState);
				}
			);

			static auto retrieve_bmes_nb = make_total_timed_event(
				std::chrono::milliseconds(114),  
				// Lambda hacking to allow the class function to be pass to 
				// make_total_timed_event. Is there any other way?
				[&](TeensyControllerState& teensyState) {
					retrieve_bmes(teensyState);
				}
			);

			// This looks intimidating but it is actually pretty simple once
			// broken down. First, by make_total_timed_event it is going to call
			// every 60 seconds the lambda that has been passed to it.
			// Inside this lambda, there are two functions: async_save but twice
			// One for PIDs, other for BMEs
			static auto save_files = make_total_timed_event(
				std::chrono::seconds(30),
				[&]() {

					spdlog::info("Saving teensy data...");
					async_save(_PIDsFile, 
						[](const PIDs& pid) {
							return 	std::to_string(pid.time) + "," +
									std::to_string(pid.PID1.Current) + "," +
									std::to_string(pid.PID1.Temperature) + "," +
									std::to_string(pid.PID2.Current) + "," +
									std::to_string(pid.PID2.Temperature) + "\n";

						}
					);

					async_save(_BMEsFile, 
						[](const BMEs& bme) {

							return 	std::to_string(bme.time) + "," +
									std::to_string(bme.LocalBME.Temperature) + "," +
									std::to_string(bme.LocalBME.Pressure) + "," +
									std::to_string(bme.LocalBME.Humidity) + "," +
									std::to_string(bme.BoxBME.Temperature) + "," +
									std::to_string(bme.BoxBME.Pressure) + "," +
									std::to_string(bme.BoxBME.Humidity) + "\n";

						}
					);
				}
			);

			// TODO(Hector): add a function that every long time (maybe 30 mins or hour)
			// Checks the status of the Teensy and sees if there are any errors

			// This should be called every 100ms
			retrieve_pids_nb(teensyState);

			// This should be called every 114ms
			retrieve_bmes_nb(teensyState);

			// These should be called every 30 seconds
			save_files();

		}

	};
} // namespace SBCQueens