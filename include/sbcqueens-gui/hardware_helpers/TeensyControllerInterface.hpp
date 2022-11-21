#ifndef TEENSYCONTROLLERINTERFACE_H
#define TEENSYCONTROLLERINTERFACE_H
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

// We need this because chrono has long names...
namespace chrono = std::chrono;

// C++ 3rd party includes
#include <armadillo>
#include <readerwriterqueue.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <toml.hpp>
using json = nlohmann::json;
// #include <boost/sml.hpp>
// namespace sml = boost::sml;

// my includes
#include "sbcqueens-gui/serial_helper.hpp"
#include "sbcqueens-gui/imgui_helpers.hpp"
#include "sbcqueens-gui/implot_helpers.hpp"
#include "sbcqueens-gui/file_helpers.hpp"
#include "sbcqueens-gui/timing_events.hpp"
#include "sbcqueens-gui/indicators.hpp"
#include "sbcqueens-gui/armadillo_helpers.hpp"

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

struct RTDs {
    double time;
    std::vector<double> RTDS;
};

struct RawRTDs {
    double time;
    std::vector<uint16_t> RTDREGS;
    std::vector<double> Resistances;
    std::vector<double> Temps;
};

struct PressureValues {
    double Pressure;
};

struct Pressures {
    double time;
    PressureValues Vacuum;
};


struct TeensySystemPars {
    uint32_t NumRtdBoards = 0;
    uint32_t NumRtdsPerBoard = 0;
    bool InRTDOnlyMode = false;
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


// end Sensors structs

enum class TeensyCommands {
    CheckError,
    GetError,
    Reset,

    SetPPIDUpdatePeriod,
    SetPPIDRTD,
    SetPPID,
    SetPPIDTripPoint,
    SetPPIDTempSetpoint,
    SetPPIDTempKp,
    SetPPIDTempTi,
    SetPPIDTempTd,
    ResetPPID,

    SetRTDSamplingPeriod,
    SetRTDMask,

    SetPeltierRelay,

    GetSystemParameters,
    GetPeltiers,
    GetRTDs,
    GetRawRTDs,
    GetPressures,
    GetBMEs,

    None = 0
};

// This map holds all the commands that the Teensy accepts, as str,
// and maps them to an enum for easy access.
const std::unordered_map<TeensyCommands, std::string> cTeensyCommands = {
    /// General system commands
    {TeensyCommands::CheckError,            "CHECKERR"},
    {TeensyCommands::Reset,                 "RESET"},
    /// !General system commands
    ////
    /// Hardware specific commands
    {TeensyCommands::SetPPIDUpdatePeriod,   "SET_PPID_UP"},
    {TeensyCommands::SetPPIDRTD,            "SET_PPID_RTD"},
    {TeensyCommands::SetPPID,               "SET_PPID"},
    {TeensyCommands::SetPPIDTripPoint,      "SET_PTRIPPOINT"},
    {TeensyCommands::SetPPIDTempSetpoint,   "SET_PTEMP"},
    {TeensyCommands::SetPPIDTempKp,         "SET_PTKP_PID"},
    {TeensyCommands::SetPPIDTempTi,         "SET_PTTi_PID"},
    {TeensyCommands::SetPPIDTempTd,         "SET_PTTd_PID"},
    {TeensyCommands::ResetPPID,             "RESET_PPID"},

    {TeensyCommands::SetRTDSamplingPeriod,  "SET_RTD_SP"},
    {TeensyCommands::SetRTDMask,            "RTD_BANK_MASK"},

    {TeensyCommands::SetPeltierRelay,       "SET_PELTIER_RELAY"},

    //// Getters
    {TeensyCommands::GetSystemParameters,   "GET_SYS_PARAMETERS"},
    {TeensyCommands::GetError,              "GETERR"},
    {TeensyCommands::GetPeltiers,           "GET_PELTIERS_CURRS"},
    {TeensyCommands::GetRTDs,               "GET_RTDS"},
    {TeensyCommands::GetRawRTDs,            "GET_RAW_RTDS"},
    {TeensyCommands::GetPressures,          "GET_PRESSURES"},
    {TeensyCommands::GetBMEs,               "GET_BMES"},
    //// !Getters
    /// !Hardware specific commands

    {TeensyCommands::None, ""}
};



// It holds everything the outside world can modify or use.
// So far, I do not like teensy_serial is here.
struct TeensyControllerData {
    std::string RunDir      = "";
    std::string RunName     = "";

    std::string Port        = "COM4";

    TeensyControllerStates CurrentState
        = TeensyControllerStates::NullState;

    TeensyCommands CommandToSend
        = TeensyCommands::None;

    TeensySystemPars SystemParameters;

    uint32_t RTDSamplingPeriod = 100;
    uint32_t RTDMask = 0xFFFF;

    // Relay stuff
    bool PeltierState       = false;

    // PID Stuff
    uint16_t PidRTD = 0;
    uint32_t PeltierPidUpdatePeriod = 100;
    bool PeltierPIDState    = false;

    float PIDTempTripPoint = 5.0;
    PIDConfig PIDTempValues;
};

using TeensyQueueType
    = std::function < bool(TeensyControllerData&) >;

// single consumer, single sender queue for Tasks of the type
// bool(TeensyControllerState&) A.K.A TeensyQueueType
using TeensyQueue
    = moodycamel::ReaderWriterQueue< TeensyQueueType >;

template<typename... Queues>
class TeensyControllerInterface {
 private:
    std::tuple<Queues&...> _queues;
    std::map<std::string, std::string> _crc_cmds;

    TeensyControllerData doe;
    IndicatorSender<IndicatorNames> TeensyIndicatorSender;
    IndicatorSender<uint16_t> MultiPlotSender;

    double _init_time;

    DataFile<Peltiers> _PeltiersFile;
    DataFile<Pressures> _PressuresFile;
    DataFile<RawRTDs> _RTDsFile;
    DataFile<BMEs> _BMEsFile;

    serial_ptr port;

 public:
    explicit TeensyControllerInterface(Queues&... queues)
        : _queues(std::forward_as_tuple(queues...)),
        TeensyIndicatorSender(std::get<GeneralIndicatorQueue&>(_queues)),
        MultiPlotSender(std::get<MultiplePlotQueue&>(_queues)) { }

    // No copying
    TeensyControllerInterface(const TeensyControllerInterface&) = delete;

    ~TeensyControllerInterface() {}

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

        spdlog::info("Initializing teensy thread");

        // GUI -> Teensy
        TeensyQueue& guiQueueOut = std::get<TeensyQueue&>(_queues);
        auto guiQueueFunc = [&guiQueueOut]() -> SBCQueens::TeensyQueueType {
            SBCQueens::TeensyQueueType new_task;
            bool success = guiQueueOut.try_dequeue(new_task);

            if (success) {
                spdlog::info("Received new teensy task");
                return new_task;
            } else {
                return [](SBCQueens::TeensyControllerData&) { return true; };
            }
        };


        // Main loop lambda
        auto main_loop = [&]() -> bool {
            TeensyQueueType task = guiQueueFunc();

            // If the queue does not return a valid function, this call will
            // do nothing and should return true always.
            // The tasks are essentially any GUI driven modification, example
            // setting the PID setpoints or constants
            // or an user driven reset
            if (!task(doe)) {
                spdlog::warn("Something went wrong with a command!");
            }
            // End Communication with the GUI

            // This will send a command only if its not none
            send_teensy_cmd(doe.CommandToSend);
            doe.CommandToSend = TeensyCommands::None;

            // This will turn into an SML soon*
            switch (doe.CurrentState) {
                case TeensyControllerStates::Standby:
                // do nothing
                // only way to get out of this state is by the GUI
                // starting a connection
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(100));
                    break;

                case TeensyControllerStates::Disconnected:
                    spdlog::warn("Manually losing connection to "
                        "Teensy with port {}", doe.Port);

                    disconnect(port);

                    // Move to standby
                    doe.CurrentState
                        = TeensyControllerStates::Standby;
                    break;

                case TeensyControllerStates::Connected:
                    // The real bread and butter of the code!
                    update();
                    break;

                case TeensyControllerStates::AttemptConnection:

                    connect_bt(port, doe.Port);

                    if (port) {
                        if (port->isOpen()) {
                            // t0 = time when the communication was 100%
                            _init_time = get_current_time_epoch();

                            // Open files to start saving!
                            open(_PressuresFile,
                                doe.RunDir
                                + "/" + doe.RunName
                                + "/Pressures.txt");
                            bool s = (_PressuresFile != nullptr);

                            open(_RTDsFile,
                                doe.RunDir
                                + "/" + doe.RunName
                                + "/RTDs.txt");
                            s = (_RTDsFile != nullptr) && s;

                            open(_PeltiersFile,
                                doe.RunDir
                                + "/" + doe.RunName
                                + "/Peltiers.txt");
                            s = (_PeltiersFile != nullptr)  && s;

                            open(_BMEsFile,
                                doe.RunDir
                                + "/" + doe.RunName
                                + "/BMEs.txt");
                            s = (_BMEsFile != nullptr)  && s;


                            if (!s) {
                                spdlog::error("Failed to open files.");
                                doe.CurrentState =
                                    TeensyControllerStates::Standby;
                            } else {
                                spdlog::info(
                                    "Connected to Teensy with port {}",
                                doe.Port);

                                send_initial_config();

                                doe.CurrentState =
                                    TeensyControllerStates::Connected;
                            }

                        } else {
                            spdlog::error("Failed to connect to port {}",
                                doe.Port);

                            doe.CurrentState
                                = TeensyControllerStates::Standby;
                        }
                    } else {
                        // No need for a message
                        doe.CurrentState
                            = TeensyControllerStates::Standby;
                    }

                    break;

                case TeensyControllerStates::Closing:
                    spdlog::info("Going to close the Teensy thread.");
                    disconnect(port);
                    return false;

                case TeensyControllerStates::NullState:
                default:
                    // do nothing other than set to standby state
                    doe.CurrentState
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
            spdlog::warn("Failed to send {0} to Teensy.",
                cTeensyCommands.at(cmd));
            flush(port);
            return;
        }

        auto msg_opt = retrieve_msg<std::string>(port);

        if (!msg_opt.has_value()) {
            spdlog::warn("Failed to retrieve data from {0} command.",
                cTeensyCommands.at(cmd));
            flush(port);
            return;
        }

        // if json_pck is empty or has the incorrect format this will fail
        json json_parse = json::parse(msg_opt.value(), nullptr, false);
        if (json_parse.is_discarded()) {
            spdlog::warn("Invalid json string. "
                        "Message received from Teensy: {0}",
                        msg_opt.value());
            flush(port);
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

        flush(port);

        retrieve_data(TeensyCommands::GetSystemParameters,
        [&](json& parse, auto& msg) {
            try {
                auto system_pars = parse.get<TeensySystemPars>();

                doe.SystemParameters = system_pars;

                spdlog::info("{0}, {1}, {2}", system_pars.NumRtdBoards,
                    system_pars.NumRtdsPerBoard, system_pars.InRTDOnlyMode);

                TeensyIndicatorSender(IndicatorNames::NUM_RTD_BOARDS,
                    doe.SystemParameters.NumRtdBoards);

                TeensyIndicatorSender(IndicatorNames::NUM_RTDS_PER_BOARD,
                    doe.SystemParameters.NumRtdsPerBoard);

                TeensyIndicatorSender(IndicatorNames::IS_RTD_ONLY,
                    doe.SystemParameters.InRTDOnlyMode);
            } catch (... ) {
                spdlog::warn("Failed to parse system data from {0}. "
                    "Message received from Teensy: {1}",
                    cTeensyCommands.at(TeensyCommands::GetSystemParameters),
                    msg.value());
                flush(port);
            }
        });

        // Flush so there is nothing in the buffer
        // making everything annoying
        flush(port);
    }

    void retrieve_pids() {
        retrieve_data(TeensyCommands::GetPeltiers,
        [&](json& parse, auto& msg) {
            try {
                auto pids = parse.get<Peltiers>();

                // Send them to GUI to draw them
                TeensyIndicatorSender(IndicatorNames::PELTIER_CURR,
                    pids.time, pids.PID.Current);

                TeensyIndicatorSender(IndicatorNames::LATEST_PELTIER_CURR,
                    pids.PID.Current);

                _PeltiersFile->Add(pids);
            } catch (... ) {
                spdlog::warn("Failed to parse latest data from {0}. "
                            "Message received from Teensy: {1}",
                            cTeensyCommands.at(TeensyCommands::GetPeltiers),
                            msg.value());
                flush(port);
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
                    MultiPlotSender(i, rtds.time, temp);

                    switch (i) {
                        case 0:
                        TeensyIndicatorSender(IndicatorNames::LATEST_RTD1_TEMP,
                            temp);
                        break;
                        case 1:
                        TeensyIndicatorSender(IndicatorNames::LATEST_RTD2_TEMP,
                            temp);
                        break;
                        case 2:
                        TeensyIndicatorSender(IndicatorNames::LATEST_RTD3_TEMP,
                            temp);
                        break;
                    }
                }

                double err = rtds.Temps[doe.PidRTD]
                    - static_cast<double>(doe.PIDTempValues.SetPoint) - 273.15;
                _error_temp_cf(err);

                TeensyIndicatorSender(IndicatorNames::PID_TEMP_ERROR,
                    arma::mean(_error_temp_cf));

                _RTDsFile->Add(rtds);
            } catch (... ) {
                spdlog::warn("Failed to parse latest data from {0}. "
                            "Message received from Teensy: {1}",
                            cTeensyCommands.at(TeensyCommands::GetRTDs),
                            msg.value());
                flush(port);
            }
        });
    }

    void retrieve_pressures() {
        retrieve_data(TeensyCommands::GetPressures,
        [&](json& parse, auto& msg) {
            try {
                auto press = parse.get<Pressures>();

                // Send them to GUI to draw them
                TeensyIndicatorSender(IndicatorNames::VACUUM_PRESS,
                    press.time , press.Vacuum.Pressure);
                TeensyIndicatorSender(IndicatorNames::LATEST_VACUUM_PRESS,
                    press.Vacuum.Pressure);

                _PressuresFile->Add(press);
            } catch (... ) {
                spdlog::warn("Failed to parse latest data from {0}. "
                            "Message received from Teensy: {1}",
                            cTeensyCommands.at(TeensyCommands::GetPressures),
                            msg.value());

                flush(port);
            }
        });
    }

    void retrieve_bmes() {
        retrieve_data(TeensyCommands::GetBMEs,
        [&](json& parse, auto& msg) {
            try {
                auto bmes = parse.get<BMEs>();

                TeensyIndicatorSender(IndicatorNames::LOCAL_BME_TEMPS,
                    bmes.time , bmes.LocalBME.Temperature);
                TeensyIndicatorSender(IndicatorNames::LOCAL_BME_PRESS,
                    bmes.time , bmes.LocalBME.Pressure);
                TeensyIndicatorSender(IndicatorNames::LOCAL_BME_HUMD,
                    bmes.time , bmes.LocalBME.Humidity);

                _BMEsFile->Add(bmes);
            } catch (... ) {
                spdlog::warn("Failed to parse latest data from {0}. "
                            "Message received from Teensy: {1}",
                            cTeensyCommands.at(TeensyCommands::GetBMEs),
                            msg.value());
                flush(port);
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
                spdlog::info("Saving teensy data...");

                async_save(_RTDsFile,
                    [](const RawRTDs& rtds) {
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

                if (!doe.SystemParameters.InRTDOnlyMode){
                    async_save(_PeltiersFile,
                        [](const Peltiers& pid) {
                            return  std::to_string(pid.time) + "," +
                                    std::to_string(pid.PID.Current) + "\n";
                    });

                    async_save(_PressuresFile,
                        [](const Pressures& press) {
                            return  std::to_string(press.time) + "," +
                                    std::to_string(press.Vacuum.Pressure) + "\n";

                    });

                    async_save(_BMEsFile,
                        [](const BMEs& bme) {
                            return  std::to_string(bme.time) + "," +
                                    std::to_string(bme.LocalBME.Temperature) + "," +
                                    std::to_string(bme.LocalBME.Pressure) + "," +
                                    std::to_string(bme.LocalBME.Humidity) + "\n";

                    });
                }
            });

        // TODO(Hector): add a function that every long time
        // (maybe 30 mins or hour)
        // Checks the status of the Teensy and sees if there are any errors

        // This should be called every 100ms
        if (!doe.SystemParameters.InRTDOnlyMode) {
            retrieve_pids_nb();
            retrieve_pres_nb();

            // This should be called every 114ms
            // retrieve_bmes_nb();
        }

        retrieve_rtds_nb();

        retrieve_rtds_nb.ChangeWaitTime(
            std::chrono::milliseconds(doe.RTDSamplingPeriod));

        // These should be called every 30 seconds
        save_files();
    }

    // Sends an specific command from the available commands
    bool send_teensy_cmd(const TeensyCommands& cmd) {
        if (!port) {
            return false;
        }

        if (cmd == TeensyCommands::None) {
            return false;
        }

        auto tcs = doe;
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
            spdlog::info("Sending command {0} to teensy!", str_cmd);
        }

        // Always add the ;\n at the end!
        str_cmd += " " + out.str();
        str_cmd += ";\n";

        return send_msg(port, str_cmd);
    }
};
}  // namespace SBCQueens
#endif
