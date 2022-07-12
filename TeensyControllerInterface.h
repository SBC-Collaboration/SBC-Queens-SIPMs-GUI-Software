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
	struct PeltierValues {
		double Current;
	};

	struct Peltiers {
		double time;
		PeltierValues PID;
	};

	struct RTDValues {
		double Temperature;
	};

	struct RTDs {
		double time;
		RTDValues RTD_1;
		RTDValues RTD_2;
	};

	struct PressureValues {
		double Pressure;
	};

	struct Pressures {
		double time;
		PressureValues Vacuum;
		PressureValues N2Line;
	};

	// The BMEs return their values as registers
	// as the calculations required to turn them into
	// temp, hum  or pressure are expensive for a MC.
	struct BMEValues {
		double Temperature;
		double Pressure;
		double Humidity;
	};

	struct BMEs {
		double time;
		BMEValues LocalBME;
		BMEValues BoxBME;
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
    void to_json(json& j, const Peltiers& p);
    // Turns a JSON to PIDs
    void from_json(const json& j, Peltiers& p);

    void to_json(json& j, const RTDs& p);
    void from_json(const json& j, RTDs& p);

    void to_json(json& j, const Pressures& p);
    void from_json(const json& j, Pressures& p);

    void to_json(json& j, const BMEs& p);
    void from_json(const json& j, BMEs& p);


	// end Sensors structs

	enum class TeensyCommands {
		CheckError,
		GetError,
		Reset,

		SetPPID,
		SetPPIDTempSetpoint,
		SetPPIDTempKp,
		SetPPIDTempTi,
		SetPPIDTempTd,
		ResetPPID,

		SetNPID,
		SetNPIDTempSetpoint,
		SetNPIDTempKp,
		SetNPIDTempTi,
		SetNPIDTempTd,
		ResetNPID,

		SetPeltierRelay,
		SetPSILimit,
		SetReleaseValve,
		SetN2Valve,

		GetPeltiers,
		GetRTDs,
		GetPressures,
		GetBMEs,

		None = 0

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
		{TeensyCommands::SetPPID, 				"SET_PPID"},
		{TeensyCommands::SetPPIDTempSetpoint, 	"SET_PTEMP"},
		{TeensyCommands::SetPPIDTempKp, 		"SET_PTKP_PID"},
		{TeensyCommands::SetPPIDTempTi, 		"SET_PTTi_PID"},
		{TeensyCommands::SetPPIDTempTd, 		"SET_PTTd_PID"},
		{TeensyCommands::ResetPPID, 			"RESET_PPID"},

		{TeensyCommands::SetNPID, 				"SET_NPID"},
		{TeensyCommands::SetNPIDTempSetpoint, 	"SET_NTEMP"},
		{TeensyCommands::SetNPIDTempKp, 		"SET_NTKP_PID"},
		{TeensyCommands::SetNPIDTempTi, 		"SET_NTTi_PID"},
		{TeensyCommands::SetNPIDTempTd, 		"SET_NTTd_PID"},
		{TeensyCommands::ResetNPID, 			"RESET_NPID"},

		{TeensyCommands::SetPeltierRelay, 		"SET_PELTIER_RELAY"},
		{TeensyCommands::SetPSILimit, 			"SET_PSI_LIMIT"},
		{TeensyCommands::SetReleaseValve, 		"REL_VALVE_STATE"},
		{TeensyCommands::SetN2Valve, 			"N2_VALVE_STATE"},

		//// Getters
		{TeensyCommands::GetError, 				"GETERR"},
		{TeensyCommands::GetPeltiers, 			"GET_PELTIERS_CURRS"},
		{TeensyCommands::GetRTDs, 				"GET_RTDS"},
		{TeensyCommands::GetPressures, 			"GET_PRESSURES"},
		{TeensyCommands::GetBMEs, 				"GET_BMES"},
		//// !Getters
		/// !Hardware specific commands

		{TeensyCommands::None, ""}
	};



	// It holds everything the outside world can modify or use.
	// So far, I do not like teensy_serial is here.
	struct TeensyControllerState {

		std::string RunDir 		= "";
		std::string RunName 	= "";

		std::string Port 		= "COM4";

		TeensyControllerStates CurrentState
			= TeensyControllerStates::NullState;

		TeensyCommands CommandToSend
			= TeensyCommands::None;

		// Relay stuff
		bool PeltierState 		= false;
		bool ReliefValveState 	= false;
		bool N2ValveState 		= false;

		// PID Stuff
		bool PeltierPIDState 	= false;
		bool LN2PIDState 		= false;

		double PSILimit 		= 5.0;
		PIDConfig PIDTempValues;
		PIDConfig NTempValues;

	};

	using TeensyQueueInType
		= std::function < bool(TeensyControllerState&) >;

	// single consumer, single sender queue for Tasks of the type
	// bool(TeensyControllerState&) A.K.A TeensyQueueInType
	using TeensyInQueue
		= moodycamel::ReaderWriterQueue< TeensyQueueInType >;

	template<typename... Queues>
	class TeensyControllerInterface {

	private:

		std::tuple<Queues&...> _queues;
		std::map<std::string, std::string> _crc_cmds;

		TeensyControllerState state_of_everything;
		IndicatorSender<IndicatorNames> TeensyIndicatorSender;

		double _init_time;

		DataFile<Peltiers> _PeltiersFile;
		DataFile<Pressures> _PressuresFile;
		DataFile<RTDs> _RTDsFile;
		DataFile<BMEs> _BMEsFile;

		serial_ptr port;

	public:

		explicit TeensyControllerInterface(Queues&... queues) 
			: _queues(forward_as_tuple(queues...)),
			TeensyIndicatorSender(std::get<SiPMsPlotQueue&>(_queues)) { }

		// No copying
		TeensyControllerInterface(const TeensyControllerInterface&) = delete;

		~TeensyControllerInterface() {}

		// Loop goes like this:
		// Initializes -> waits for action from GUI to connect 
		//	-> Update (every dt or at f) (retrieves info or updates teensy) ->
		//  -> Update until disconnect, close, or error.
		void operator()() {

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

				// This will send a command only if its not none
				send_teensy_cmd(state_of_everything.CommandToSend);
				state_of_everything.CommandToSend = TeensyCommands::None;

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

						disconnect(port);

						// Move to standby
						state_of_everything.CurrentState 
							= TeensyControllerStates::Standby;
						break;

					case TeensyControllerStates::Connected:
						// The real bread and butter of the code!
						update(state_of_everything);
						break;

					case TeensyControllerStates::AttemptConnection:
						
						connect_bt(port, state_of_everything.Port);

						if(port) {
							if(port->isOpen()) {

								// t0 = time when the communication was 100%
								_init_time = get_current_time_epoch();

								// Open files to start saving!
								open(_PeltiersFile,
									state_of_everything.RunDir
									+ "/" + state_of_everything.RunName
									+ "/Peltiers.txt");
						 		bool s = _PeltiersFile > 0;

								open(_PressuresFile,
									state_of_everything.RunDir
									+ "/" + state_of_everything.RunName
									+ "/Pressures.txt");
						 		s &= _PeltiersFile > 0;

								open(_RTDsFile,
									state_of_everything.RunDir
									+ "/" + state_of_everything.RunName
									+ "/RTDs.txt");
						 		s &= _RTDsFile > 0;

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
					[&](auto cmd){
						return send_teensy_cmd(cmd);
					}
				);


			send_ts_cmd_b(TeensyCommands::SetPPIDTempSetpoint);
			send_ts_cmd_b(TeensyCommands::SetPPIDTempKp);
			send_ts_cmd_b(TeensyCommands::SetPPIDTempTd);
			send_ts_cmd_b(TeensyCommands::SetPPIDTempTi);

			send_ts_cmd_b(TeensyCommands::SetNPIDTempSetpoint);
			send_ts_cmd_b(TeensyCommands::SetNPIDTempKp);
			send_ts_cmd_b(TeensyCommands::SetNPIDTempTd);
			send_ts_cmd_b(TeensyCommands::SetNPIDTempTi);

			// Flush so there is nothing in the buffer
			// making everything annoying
			flush(port);
		}

		template <class T>
		void retrieve_data(TeensyControllerState& teensyState,
			const TeensyCommands& cmd, T&& f) {

			if(!send_teensy_cmd(cmd)) {
				spdlog::warn("Failed to send {0} to Teensy.",
					cTeensyCommands.at(cmd));
				flush(port);
				return;
			}

			auto msg_opt = retrieve_msg<std::string>(port);

			if(!msg_opt.has_value()) {
				spdlog::warn("Failed to retrieve data from {0} command.",
					cTeensyCommands.at(cmd));
				flush(port);
				return;
			}

			// if json_pck is empty or has the incorrect format this will fail
			json json_parse = json::parse(msg_opt.value(), nullptr, false);
			if(json_parse.is_discarded()) {
				spdlog::warn("Invalid json string. "
							"Message received from Teensy: {0}", msg_opt.value());
				flush(port);
				return;
			}

			f(json_parse, msg_opt);

		}

		void retrieve_pids(TeensyControllerState& teensyState) {

			retrieve_data(teensyState, TeensyCommands::GetPeltiers,
			[&](json& parse, auto& msg) {
				try {

					auto pids = parse.get<Peltiers>();

					// Time since communication with the Teensy
					double dt = (pids.time - _init_time) / 1000.0;

					// Send them to GUI to draw them
					TeensyIndicatorSender(IndicatorNames::PELTIER_CURR,
						dt, pids.PID.Current);

					TeensyIndicatorSender(IndicatorNames::LATEST_Peltier_CURR,
						pids.PID.Current);

					_PeltiersFile->Add(pids);

				} catch (... ) {
					spdlog::warn("Failed to parse latest data from {0}. "
								"Message received from Teensy: {1}",
								cTeensyCommands.at(TeensyCommands::GetPeltiers),
								msg.value());
				}
			});

		}

		void retrieve_rtds(TeensyControllerState& teensyState) {
			retrieve_data(teensyState, TeensyCommands::GetRTDs,
			[&](json& parse, auto& msg) {
				try {

					auto rtds = parse.get<RTDs>();

					// Time since communication with the Teensy
					double dt = (rtds.time - _init_time) / 1000.0;

					// Send them to GUI to draw them
					TeensyIndicatorSender(IndicatorNames::RTD_TEMP_ONE,
						dt, rtds.RTD_1.Temperature);
					TeensyIndicatorSender(IndicatorNames::LATEST_RTD1_TEMP,
						rtds.RTD_1.Temperature);

					TeensyIndicatorSender(IndicatorNames::RTD_TEMP_TWO,
						dt, rtds.RTD_2.Temperature);
					TeensyIndicatorSender(IndicatorNames::LATEST_RTD2_TEMP,
						rtds.RTD_2.Temperature);

					_RTDsFile->Add(rtds);

				} catch (... ) {
					spdlog::warn("Failed to parse latest data from {0}. "
								"Message received from Teensy: {1}",
								cTeensyCommands.at(TeensyCommands::GetRTDs),
								msg.value());
				}

			});
		}

		void retrieve_pressures(TeensyControllerState& teensyState) {
			retrieve_data(teensyState, TeensyCommands::GetPressures,
			[&](json& parse, auto& msg) {
				try {

					auto press = parse.get<Pressures>();

					// Time since communication with the Teensy
					double dt = (press.time - _init_time) / 1000.0;

					// Send them to GUI to draw them
					TeensyIndicatorSender(IndicatorNames::VACUUM_PRESS,
						dt, press.Vacuum.Pressure);
					TeensyIndicatorSender(IndicatorNames::LATEST_VACUUM_PRESS,
						press.Vacuum.Pressure);

					TeensyIndicatorSender(IndicatorNames::NTWO_PRESS,
						dt, press.N2Line.Pressure);
					TeensyIndicatorSender(IndicatorNames::LATEST_N2_PRESS,
						press.N2Line.Pressure);

					_PressuresFile->Add(press);

				} catch (... ) {
					spdlog::warn("Failed to parse latest data from {0}. "
								"Message received from Teensy: {1}",
								cTeensyCommands.at(TeensyCommands::GetPressures),
								msg.value());
				}

			});
		}

		void retrieve_bmes(TeensyControllerState& teensyState) {

			retrieve_data(teensyState, TeensyCommands::GetBMEs,
			[&](json& parse, auto& msg) {
				try {

					auto bmes = parse.get<BMEs>();

					// Time since communication with the Teensy
					double dt = (bmes.time - _init_time) / 1000.0;

					TeensyIndicatorSender(IndicatorNames::LOCAL_BME_Temps,
						dt, bmes.LocalBME.Temperature);
					TeensyIndicatorSender(IndicatorNames::LOCAL_BME_Pressure,
						dt, bmes.LocalBME.Pressure);
					TeensyIndicatorSender(IndicatorNames::LOCAL_BME_Humidity,
						dt, bmes.LocalBME.Humidity);

					TeensyIndicatorSender(IndicatorNames::BOX_BME_Temps,
						dt, bmes.BoxBME.Temperature);
					TeensyIndicatorSender(IndicatorNames::BOX_BME_Pressure,
						dt, bmes.BoxBME.Pressure);
					TeensyIndicatorSender(IndicatorNames::BOX_BME_Humidity,
						dt, bmes.BoxBME.Humidity);

					TeensyIndicatorSender(IndicatorNames::LATEST_BOX_BME_HUM,
						bmes.BoxBME.Humidity);
					TeensyIndicatorSender(IndicatorNames::LATEST_BOX_BME_TEMP,
						bmes.BoxBME.Temperature);

					_BMEsFile->Add(bmes);

				} catch (... ) {
					spdlog::warn("Failed to parse latest data from {0}. "
								"Message received from Teensy: {1}",
								cTeensyCommands.at(TeensyCommands::GetBMEs),
								msg.value());
				}
			});

		}

		// It continuosly polls the Teensy for the latest data and saves it
		// to the file and updates the GUI graphs
		void update(TeensyControllerState& teensyState) {

			// We do not need this function to be out of this scope
			// and make it static to call it once
			static auto retrieve_pids_nb = make_total_timed_event(
				std::chrono::milliseconds(500),
				// Lambda hacking to allow the class function to be pass to 
				// make_total_timed_event. Is there any other way?
				[&](TeensyControllerState& teensyState) {
					retrieve_pids(teensyState);
				}
			);

			static auto retrieve_rtds_nb = make_total_timed_event(
				std::chrono::milliseconds(100),
				// Lambda hacking to allow the class function to be pass to
				// make_total_timed_event. Is there any other way?
				[&](TeensyControllerState& teensyState) {
					retrieve_rtds(teensyState);
				}
			);

			static auto retrieve_pres_nb = make_total_timed_event(
				std::chrono::milliseconds(500),
				// Lambda hacking to allow the class function to be pass to
				// make_total_timed_event. Is there any other way?
				[&](TeensyControllerState& teensyState) {
					retrieve_pressures(teensyState);
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
					async_save(_PeltiersFile,
						[](const Peltiers& pid) {
							return 	std::to_string(pid.time) + "," +
									std::to_string(pid.PID.Current) + "\n";

						}
					);

					async_save(_RTDsFile,
						[](const RTDs& rtds) {
							return 	std::to_string(rtds.time) + "," +
									std::to_string(rtds.RTD_1.Temperature) + "," +
									std::to_string(rtds.RTD_2.Temperature) + "\n";

						}
					);

					async_save(_PressuresFile,
						[](const Pressures& press) {
							return 	std::to_string(press.time) + "," +
									std::to_string(press.Vacuum.Pressure) + "," +
									std::to_string(press.N2Line.Pressure) + "\n";

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
			retrieve_rtds_nb(teensyState);
			retrieve_pres_nb(teensyState);

			// This should be called every 114ms
			retrieve_bmes_nb(teensyState);

			// These should be called every 30 seconds
			save_files();

		}

		// Sends an specific command from the available commands
		bool send_teensy_cmd(const TeensyCommands& cmd) {

			if(!port) {
				return false;
			}

			if(cmd == TeensyCommands::None) {
				return false;
			}

			auto tcs = state_of_everything;
			// This functions essentially wraps the send_msg function with
			// the map to get the command from the map.
			auto str_cmd = cTeensyCommands.at(cmd);

			std::ostringstream out;
			out.precision(6);
			out << std::defaultfloat;

			switch(cmd) {
				case TeensyCommands::SetPPID:
					out << tcs.PeltierPIDState;
				break;

				case TeensyCommands::SetPPIDTempSetpoint:
					out << tcs.PIDTempValues.SetPoint;
				break;

				case TeensyCommands::SetPPIDTempKp:
					out << tcs.PIDTempValues.Kp;
				break;

				case TeensyCommands::SetPPIDTempTi:
					out << tcs.PIDTempValues.Ti;
				break;

				case TeensyCommands::SetPPIDTempTd:
					out << tcs.PIDTempValues.Td;
				break;

				case TeensyCommands::SetNPID:
					out << tcs.LN2PIDState;
				break;

				case TeensyCommands::SetNPIDTempSetpoint:
					out << tcs.NTempValues.SetPoint;
				break;

				case TeensyCommands::SetNPIDTempKp:
					out << tcs.NTempValues.Kp;
				break;

				case TeensyCommands::SetNPIDTempTi:
					out << tcs.NTempValues.Ti;
				break;

				case TeensyCommands::SetNPIDTempTd:
					out << tcs.NTempValues.Td;
				break;

				case TeensyCommands::SetPeltierRelay:
					out << tcs.PeltierState;
				break;

				case TeensyCommands::SetPSILimit:
					out << tcs.PSILimit;
				break;

				case TeensyCommands::SetReleaseValve:
					out << tcs.ReliefValveState;
				break;

				case TeensyCommands::SetN2Valve:
					out << tcs.N2ValveState;
				break;

				// The default case assumes all the commands not mention above
				// do not have any inputs
				default:
				break;

			}

			if(cmd != TeensyCommands::GetBMEs
				&& cmd != TeensyCommands::GetRTDs
				&& cmd != TeensyCommands::GetPeltiers
				&& cmd != TeensyCommands::GetPressures) {
				spdlog::info("Sending command {0} to teensy!", str_cmd);
			}

			// Always add the ;\n at the end!
			str_cmd += " " + out.str();
			str_cmd += ";\n";

			return send_msg(port, str_cmd);
		}

	};
} // namespace SBCQueens
