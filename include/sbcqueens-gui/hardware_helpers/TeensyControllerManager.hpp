#ifndef TEENSYCONTROLLERMANAGER_H
#define TEENSYCONTROLLERMANAGER_H
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

// C STD includes
// C 3rd party includes

// C++ STD includes
#include <cstdint>
#include <tuple>
#include <vector>
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
#include <filesystem>

// We need this because chrono has long names...
namespace chrono = std::chrono;

// C++ 3rd party includes
#include <date/date.h>
#include <armadillo>
#include <readerwriterqueue.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <toml.hpp>
using json = nlohmann::json;
// #include <boost/sml.hpp>
// namespace sml = boost::sml;

// my includes
#include "sbcqueens-gui/multithreading_helpers/ThreadManager.hpp"

#include "sbcqueens-gui/serial_helper.hpp"
#include "sbcqueens-gui/imgui_helpers.hpp"
#include "sbcqueens-gui/implot_helpers.hpp"
#include "sbcqueens-gui/file_helpers.hpp"
#include "sbcqueens-gui/timing_events.hpp"
#include "sbcqueens-gui/armadillo_helpers.hpp"

#include "sbcqueens-gui/hardware_helpers/TeensyControllerData.hpp"

namespace SBCQueens {

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

enum class BME_TYPE {LOCAL};
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
    {BME_TYPE::LOCAL, LOCAL_COMPENSATION}
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

double __calibrate_curve(const size_t& i, const uint16_t& count);
double register_to_T90(const double& Rtf);

// These functions are required for json to convert to our types.
// Turns PIDs type to Json.
void to_json(json& j, const Peltiers& p);
// Turns a JSON to PIDs
void from_json(const json& j, Peltiers& p);

void to_json(json& j, const RTDs& p);
void from_json(const json& j, RTDs& p);

void to_json(json& j, const RawRTDs& p);
void from_json(const json& j, RawRTDs& p);

void to_json(json& j, const TeensySystemPars& p);
void from_json(const json& j, TeensySystemPars& p);


void to_json(json& j, const Pressures& p);
void from_json(const json& j, Pressures& p);

void to_json(json& j, const BMEs& p);
void from_json(const json& j, BMEs& p);

template<typename Pipes>
class TeensyControllerManager : public ThreadManager<Pipes> {
    std::map<std::string, std::string> _crc_cmds;

    using TeensyPipe_type = typename Pipes::TeensyPipe_type;
    TeensyControllerPipeEnd<TeensyPipe_type, PipeEndType::Consumer> _teensy_pipe_end;
    TeensyControllerData& _doe;

    std::shared_ptr<spdlog::logger> _logger;

    double _init_time;

    std::string _run_name     = "";
    std::shared_ptr<DataFile<Peltiers>> _peltiers_file;
    std::shared_ptr<DataFile<Pressures>> _pressures_file;
    std::shared_ptr<DataFile<RawRTDs>> _RTDs_file;
    std::shared_ptr<DataFile<BMEs>> _BMEs_file;

    serial_ptr _port;

 public:
    explicit TeensyControllerManager(const Pipes& pipes) :
        ThreadManager<Pipes>{pipes},
        _teensy_pipe_end(pipes.TeensyPipe), _doe{_teensy_pipe_end.Data}
        // _indicator_sender(std::get<GeneralIndicatorQueue&>(_queues)),
        // _plot_sender(std::get<MultiplePlotQueue&>(_queues)) { }
    {
        _logger = spdlog::get("log");
        _logger->info("Initializing teensy thread");
    }

    ~TeensyControllerManager() {
        _logger->info("Closing Teensy Controller Manager");
    }

    // Loop goes like this:
    // Initializes -> waits for action from GUI to connect
    //  -> Update (every dt or at f) (retrieves info or updates teensy) ->
    //  -> Update until disconnect, close, or error.
    void operator()() {
        auto connect_bt = make_blocking_total_timed_event(
            std::chrono::milliseconds(5000),
            [&](serial_ptr& p, const std::string& port_name) {
                connect(p, port_name);
            });

        // Main loop lambda
        auto main_loop = [&]() -> bool {
            TeensyControllerData new_task;
            // If the queue does not return a valid callback, this call will
            // do nothing and should return true always.
            // The tasks are essentially any GUI driven modification, example
            // setting the PID setpoints or constants
            // or an user driven reset
            if (_teensy_pipe_end.retrieve(new_task)) {
                new_task.Callback(_doe);
            }
            // End Communication with the GUI

            // This will send a command only if its not none
            send_teensy_cmd(_doe.CommandToSend);
            _doe.CommandToSend = TeensyCommands::None;

            // This will turn into an SML soon*
            switch (_doe.CurrentState) {
                case TeensyControllerStates::Standby:
                // do nothing
                // only way to get out of this state is by the GUI
                // starting a connection
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(100));
                    break;

                case TeensyControllerStates::Disconnected:
                    _logger->warn("Manually losing connection to "
                        "Teensy with port {}", _doe.Port);

                    disconnect(_port);

                    // Move to standby
                    _doe.CurrentState
                        = TeensyControllerStates::Standby;
                    break;

                case TeensyControllerStates::Connected:
                    // The real bread and butter of the code!
                    update();
                    break;

                case TeensyControllerStates::AttemptConnection:

                    connect_bt(_port, _doe.Port);

                    if (_port) {
                        if (_port->isOpen()) {
                            // t0 = time when the communication was 100%
                            _init_time = get_current_time_epoch();

                            // These lines get today's date and creates a folder
                            // under that date
                            // There is a similar code in the CAEN
                            // interface file
                            std::ostringstream out;
                            auto today = date::year_month_day{
                                date::floor<date::days>(
                                    std::chrono::system_clock::now())};
                            out << today;
                            _run_name = out.str();

                            std::filesystem::create_directory(_doe.RunDir
                                + "/" + _run_name);

                            // Open files to start saving!
                            _pressures_file = std::make_shared<DataFile<Pressures>>(
                                _doe.RunDir
                                + "/" + _run_name
                                + "/Pressures.txt");
                            bool s = _pressures_file->isOpen();

                            _RTDs_file = std::make_shared<DataFile<RawRTDs>>(
                                _doe.RunDir
                                + "/" + _run_name
                                + "/RTDs.txt");
                            s = _RTDs_file->isOpen() && s;

                            _peltiers_file = std::make_shared<DataFile<Peltiers>>(
                                _doe.RunDir
                                + "/" + _run_name
                                + "/Peltiers.txt");
                            s = _peltiers_file->isOpen() && s;

                            _BMEs_file = std::make_shared<DataFile<BMEs>>(
                                _doe.RunDir
                                + "/" + _run_name
                                + "/BMEs.txt");
                            s = _BMEs_file->isOpen() && s;


                            if (!s) {
                                _logger->error("Failed to open files.");
                                _doe.CurrentState =
                                    TeensyControllerStates::Standby;
                            } else {
                                _logger->info(
                                    "Connected to Teensy with port {}",
                                _doe.Port);

                                send_initial_config();

                                _doe.CurrentState =
                                    TeensyControllerStates::Connected;
                            }

                        } else {
                            _logger->error("Failed to connect to port {}",
                                _doe.Port);

                            _doe.CurrentState
                                = TeensyControllerStates::Standby;
                        }
                    } else {
                        // No need for a message
                        _doe.CurrentState
                            = TeensyControllerStates::Standby;
                    }

                    break;

                case TeensyControllerStates::Closing:
                    _logger->info("Going to close the Teensy thread.");
                    disconnect(_port);
                    return false;

                default:
                    // do nothing other than set to standby state
                    _doe.CurrentState
                        = TeensyControllerStates::Standby;
            }

            // Only one case, the Closing state will return false;
            return true;
            // * = As soon as I feel learning that library is worth it.
        };

        // This will call rlf_block_time every 1 ms
        // microseconds to have better resolution or less error perhaps
        auto main_loop_block_time = make_blocking_total_timed_event(
            std::chrono::microseconds(1000), main_loop);

        // Actual loop!
        while (main_loop_block_time()) { }
    }  // here port should get deleted

 private:

    template <class T>
    void retrieve_data(const TeensyCommands& cmd, T&& f) {
        if (!send_teensy_cmd(cmd)) {
            _logger->warn("Failed to send {0} to Teensy.",
                cTeensyCommands.at(cmd));
            flush(_port);
            return;
        }

        auto msg_opt = retrieve_msg<std::string>(_port);

        if (!msg_opt.has_value()) {
            _logger->warn("Failed to retrieve data from {0} command.",
                cTeensyCommands.at(cmd));
            flush(_port);
            return;
        }

        // if json_pck is empty or has the incorrect format this will fail
        json json_parse = json::parse(msg_opt.value(), nullptr, false);
        if (json_parse.is_discarded()) {
            _logger->warn("Invalid json string. "
                        "Message received from Teensy: {0}",
                        msg_opt.value());
            flush(_port);
            return;
        }

        f(json_parse, msg_opt);
    }

    void send_initial_config() {
        auto send_ts_cmd_b = make_blocking_total_timed_event(
            std::chrono::milliseconds(10),
            [&](auto cmd){
                return send_teensy_cmd(cmd);
        });

        send_ts_cmd_b(TeensyCommands::SetRTDSamplingPeriod);
        send_ts_cmd_b(TeensyCommands::SetRTDMask);

        send_ts_cmd_b(TeensyCommands::SetPPIDTempSetpoint);
        send_ts_cmd_b(TeensyCommands::SetPPIDTripPoint);
        send_ts_cmd_b(TeensyCommands::SetPPIDTempKp);
        send_ts_cmd_b(TeensyCommands::SetPPIDTempTd);
        send_ts_cmd_b(TeensyCommands::SetPPIDTempTi);

        flush(_port);

        retrieve_data(TeensyCommands::GetSystemParameters,
        [&](json& parse, auto& msg) {
            try {
                auto system_pars = parse.get<TeensySystemPars>();

                _doe.SystemParameters = system_pars;

                _logger->info("{0}, {1}, {2}", system_pars.NumRtdBoards,
                    system_pars.NumRtdsPerBoard, system_pars.InRTDOnlyMode);

            } catch (... ) {
                _logger->warn("Failed to parse system data from {0}. "
                    "Message received from Teensy: {1}",
                    cTeensyCommands.at(TeensyCommands::GetSystemParameters),
                    msg.value());
                flush(_port);
            }
        });

        // Flush so there is nothing in the buffer
        // making everything annoying
        flush(_port);
    }

    void retrieve_pids() {
        retrieve_data(TeensyCommands::GetPeltiers,
        [&](json& parse, auto& msg) {
            try {
                auto pids = parse.get<Peltiers>();

                // Send them to GUI to draw them
                // _indicator_sender(IndicatorNames::PELTIER_CURR,
                //     pids.time, pids.PID.Current);

                // _indicator_sender(IndicatorNames::LATEST_PELTIER_CURR,
                //     pids.PID.Current);

                _peltiers_file->add(pids);
            } catch (... ) {
                _logger->warn("Failed to parse latest data from {0}. "
                            "Message received from Teensy: {1}",
                            cTeensyCommands.at(TeensyCommands::GetPeltiers),
                            msg.value());
                flush(_port);
            }
        });
    }

    void retrieve_rtds() {
        static CircularBuffer<100> _error_temp_cf;
        retrieve_data(TeensyCommands::GetRawRTDs,
        [&](json& parse, auto& msg) {
            try {
                auto rtds = parse.get<RawRTDs>();

                // Send them to GUI to draw them
                for (uint16_t i = 0; i < rtds.Temps.size(); i++) {
                    const double temp = rtds.Temps[i];
                    // _plot_sender(i, rtds.time, temp);

                     switch (i) {
                         case 0:
                            _doe.RTD1Temp = temp;
                         break;
                         case 1:
                             _doe.RTD2Temp = temp;
                         break;
                         case 2:
                             _doe.RTD3Temp = temp;
                         break;
                     }


                }

                _doe.TemperatureData(get_current_time_epoch(),
                                     rtds.Temps[0],
                                     rtds.Temps[1],
                                     rtds.Temps[2]);

                double err = rtds.Temps[_doe.PidRTD]
                    - static_cast<double>(_doe.PIDTempValues.SetPoint) - 273.15;
                _error_temp_cf.Add(err);


                _RTDs_file->add(rtds);
            } catch (... ) {
                _logger->warn("Failed to parse latest data from {0}. "
                            "Message received from Teensy: {1}",
                            cTeensyCommands.at(TeensyCommands::GetRTDs),
                            msg.value());
                flush(_port);
            }
        });
    }

    void retrieve_pressures() {
        retrieve_data(TeensyCommands::GetPressures,
        [&](json& parse, auto& msg) {
            try {
                auto press = parse.get<Pressures>();

                // Send them to GUI to draw them
                // _indicator_sender(IndicatorNames::VACUUM_PRESS,
                //     press.time , press.Vacuum.Pressure);
                // _indicator_sender(IndicatorNames::LATEST_VACUUM_PRESS,
                //     press.Vacuum.Pressure);

                _pressures_file->add(press);
            } catch (... ) {
                _logger->warn("Failed to parse latest data from {0}. "
                            "Message received from Teensy: {1}",
                            cTeensyCommands.at(TeensyCommands::GetPressures),
                            msg.value());

                flush(_port);
            }
        });
    }

    void retrieve_bmes() {
        retrieve_data(TeensyCommands::GetBMEs,
        [&](json& parse, auto& msg) {
            try {
                auto bmes = parse.get<BMEs>();

                // _indicator_sender(IndicatorNames::LOCAL_BME_TEMPS,
                //     bmes.time , bmes.LocalBME.Temperature);
                // _indicator_sender(IndicatorNames::LOCAL_BME_PRESS,
                //     bmes.time , bmes.LocalBME.Pressure);
                // _indicator_sender(IndicatorNames::LOCAL_BME_HUMD,
                //     bmes.time , bmes.LocalBME.Humidity);

                _BMEs_file->add(bmes);
            } catch (... ) {
                _logger->warn("Failed to parse latest data from {0}. "
                            "Message received from Teensy: {1}",
                            cTeensyCommands.at(TeensyCommands::GetBMEs),
                            msg.value());
                flush(_port);
            }
        });
    }

    // It continuosly polls the Teensy for the latest data and saves it
    // to the file and updates the GUI graphs
    void update() {
        // We do not need this function to be out of this scope
        // and make it static to call it once
        static auto retrieve_pids_nb = make_total_timed_event(
            std::chrono::milliseconds(1000),
            // Lambda hacking to allow the class function to be pass to
            // make_total_timed_event. Is there any other way?
            [&]() {
                retrieve_pids();
        });

        static auto retrieve_rtds_nb = make_total_timed_event(
            std::chrono::milliseconds(1000),
            // Lambda hacking to allow the class function to be pass to
            // make_total_timed_event. Is there any other way?
            [&]() {
                retrieve_rtds();
        });

        static auto retrieve_pres_nb = make_total_timed_event(
            std::chrono::milliseconds(1000),
            // Lambda hacking to allow the class function to be pass to
            // make_total_timed_event. Is there any other way?
            [&]() {
                retrieve_pressures();
        });

        // static auto retrieve_bmes_nb = make_total_timed_event(
        //     std::chrono::milliseconds(114),
        //     // Lambda hacking to allow the class function to be pass to
        //     // make_total_timed_event. Is there any other way?
        //     [&]() {
        //         retrieve_bmes();
        // });

        // This looks intimidating but it is actually pretty simple once
        // broken down. First, by make_total_timed_event it is going to call
        // every 60 seconds the lambda that has been passed to it.
        // Inside this lambda, there are two functions: async_save but twice
        // One for PIDs, other for BMEs
        static auto save_files = make_total_timed_event(
            std::chrono::seconds(30),
            [&]() {
                _logger->info("Saving teensy data...");

                _RTDs_file->async_save([](const RawRTDs& rtds) {
                    std::ostringstream out;
                    for (std::size_t i = 0; i < rtds.RTDREGS.size(); i++) {
                        out << rtds.Resistances[i] << ","
                            << rtds.Temps[i];

                        if (i == rtds.RTDREGS.size() - 1) {
                            out << '\n';
                        } else {
                            out << ',';
                        }
                    }

                    return  std::to_string(rtds.time) + "," + out.str();
                });


                if (!_doe.SystemParameters.InRTDOnlyMode){
                    _peltiers_file->async_save(
                        [](const Peltiers& pid) {
                                 return  std::to_string(pid.time) + "," +
                                         std::to_string(pid.PID.Current) + "\n";
                        }
                    );

                    _pressures_file->async_save(
                        [](const Pressures& press) {
                             return  std::to_string(press.time) + "," +
                                     std::to_string(press.Vacuum.Pressure) + "\n";
                        }
                    );

                    _BMEs_file->async_save(
                        [](const BMEs& bme) {
                             return  std::to_string(bme.time) + "," +
                                     std::to_string(bme.LocalBME.Temperature) + "," +
                                     std::to_string(bme.LocalBME.Pressure) + "," +
                                     std::to_string(bme.LocalBME.Humidity) + "\n";
                        }
                    );
                }
            });

        // TODO(Hector): add a function that every long time
        // (maybe 30 mins or hour)
        // Checks the status of the Teensy and sees if there are any errors

        // This should be called every 100ms
        if (!_doe.SystemParameters.InRTDOnlyMode) {
            retrieve_pids_nb();
            retrieve_pres_nb();

            // This should be called every 114ms
            // retrieve_bmes_nb();
        }

        retrieve_rtds_nb();

        retrieve_rtds_nb.ChangeWaitTime(
            std::chrono::milliseconds(_doe.RTDSamplingPeriod));

        // These should be called every 30 seconds
        save_files();
    }

    // Sends an specific command from the available commands
    bool send_teensy_cmd(const TeensyCommands& cmd) {
        if (!_port) {
            return false;
        }

        if (cmd == TeensyCommands::None) {
            return false;
        }

        const auto& tcs = _doe;
        // This functions essentially wraps the send_msg function with
        // the map to get the command from the map.
        auto str_cmd = cTeensyCommands.at(cmd);

        std::ostringstream out;
        out.precision(6);
        out << std::defaultfloat;

        switch (cmd) {
            case TeensyCommands::SetPPIDUpdatePeriod:
                out << tcs.PeltierPidUpdatePeriod;
            break;

            case TeensyCommands::SetPPIDRTD:
                out << tcs.PidRTD;
            break;

            case TeensyCommands::SetPPID:
                out << tcs.PeltierPIDState;
            break;

            case TeensyCommands::SetPPIDTempSetpoint:
                out << tcs.PIDTempValues.SetPoint;
            break;

            case TeensyCommands::SetPPIDTripPoint:
                out << tcs.PIDTempTripPoint;
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

            case TeensyCommands::SetRTDSamplingPeriod:
                out << tcs.RTDSamplingPeriod;
            break;

            case TeensyCommands::SetRTDMask:
                out << tcs.RTDMask;
            break;

            case TeensyCommands::SetPeltierRelay:
                out << tcs.PeltierState;

            // The default case assumes all the commands not mention above
            // do not have any inputs
            default:
            break;
        }

        if (cmd != TeensyCommands::GetBMEs
            && cmd != TeensyCommands::GetRTDs
            && cmd != TeensyCommands::GetRawRTDs
            && cmd != TeensyCommands::GetPeltiers
            && cmd != TeensyCommands::GetPressures) {
            _logger->info("Sending command {0} to teensy!", str_cmd);
        }

        // Always add the ;\n at the end!
        str_cmd += " " + out.str();
        str_cmd += ";\n";

        return send_msg(_port, str_cmd);
    }
};

template<typename Pipes>
auto make_teensy_controller_manager(const Pipes& p) {
    return std::make_unique<TeensyControllerManager<Pipes>>(p);
}

}  // namespace SBCQueens
#endif
