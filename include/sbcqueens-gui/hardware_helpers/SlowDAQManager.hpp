#ifndef SlowDAQManager_H
#define SlowDAQManager_H
#pragma once

/*

    This class is intended to be used to hold miscellaneous devices
    which time scales are in seconds or higher.

    Example: pressure transducers

*/

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <string>
#include <tuple>
#include <vector>

// C++ 3rd party includes
// my includes
#include "sbcqueens-gui/multithreading_helpers/ThreadManager.hpp"

#include "sbcqueens-gui/serial_helper.hpp"
#include "sbcqueens-gui/file_helpers.hpp"
#include "sbcqueens-gui/timing_events.hpp"

#include "sbcqueens-gui/hardware_helpers/SlowDAQData.hpp"

namespace SBCQueens {

template<typename Pipes>
class SlowDAQManager : public ThreadManager<Pipes> {
    using SlowDAQ_type = typename Pipes::SlowDAQ_type;
    SlowDAQPipeEnd<SlowDAQ_type, PipeEndType::Consumer> _slowdaq_pipe_end;
    SlowDAQData& _slowdaq_doe;

    std::shared_ptr<spdlog::logger> _logger;

    std::shared_ptr<DataFile<PFEIFFERSingleGaugeData>> _pfeiffer_file;

    std::string _run_name;

    // PFEIFFER Single Gauge port
    serial_ptr _pfeiffers_port;
    double _init_time = 0;

 public:
    explicit SlowDAQManager(const Pipes& p) :
        ThreadManager<Pipes>{p},
        _slowdaq_pipe_end(p.SlowDAQPipe), _slowdaq_doe{_slowdaq_pipe_end.Data}
        // _plot_sender(std::get<GeneralIndicatorQueue&>(_queues))
    {
        _logger = spdlog::get("log");
    }

    ~SlowDAQManager() {
        _logger->info("Closing SlowDAQ Manager.");
    }

    void operator()() {
        _logger->info("Initializing slow DAQ thread...");
        _logger->info("Slow DAQ components: PFEIFFERSingleGauge");

        auto send_data_tt = make_total_timed_event(
                std::chrono::milliseconds(200),
                [&]() {
                    _slowdaq_pipe_end.send();
                }
        );

        auto main_loop = [&]() -> bool {

            SlowDAQData new_task;
            // If the queue does not return a valid callback, this call will
            // do nothing and should return true always.
            // The tasks are essentially any GUI driven modification, example
            // setting the PID setpoints or constants
            // or an user driven reset
            if (_slowdaq_pipe_end.retrieve(new_task)) {
                new_task.Callback(_slowdaq_doe);
            }
            send_data_tt();
            // End Communication with the GUI

            if (!_slowdaq_doe.PFEIFFERSingleGaugeEnable) {
                if (_slowdaq_doe.PFEIFFERState == PFEIFFERSSGState::Closing) {
                    return false;
                }

                return true;
            }

            switch (_slowdaq_doe.PFEIFFERState) {
                case PFEIFFERSSGState::AttemptConnection:
                {
                    SerialParams ssp;
                    ssp.Timeout = serial::Timeout::simpleTimeout(10);
                    connect_par(_pfeiffers_port, _slowdaq_doe.PFEIFFERPort, ssp);

                    if (_pfeiffers_port) {
                        if (_pfeiffers_port->isOpen()) {

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

                            std::filesystem::create_directory(_slowdaq_doe.RunDir
                                + "/" + _run_name);

                            _init_time = get_current_time_epoch();
                            _pfeiffer_file = std::make_shared<DataFile<PFEIFFERSingleGaugeData>>(
                                _slowdaq_doe.RunDir
                                + "/" + _run_name
                                + "/PFEIFFERSSPressures.txt");
                            bool s = _pfeiffer_file->isOpen();

                            if (not s) {
                                _logger->error("Failed to open files.");
                                _slowdaq_doe.PFEIFFERState =
                                    PFEIFFERSSGState::Standby;
                            } else {
                                    _logger->info(
                                        "Connected to PFEIFFERS with port {}",
                                    _slowdaq_doe.PFEIFFERPort);

                                    send_msg(_pfeiffers_port, "COM," +
                                        std::to_string(static_cast<uint16_t> (
                                            _slowdaq_doe.PFEIFFERSingleGaugeUpdateSpeed)) + "\n",
                                        "");


                                    flush(_pfeiffers_port);

                                    _logger->info("Sent {0} ", "COM," +
                                        std::to_string(static_cast<uint16_t> (
                                            _slowdaq_doe.PFEIFFERSingleGaugeUpdateSpeed)));

                                    _slowdaq_doe.PFEIFFERState =
                                        PFEIFFERSSGState::Connected;
                                }

                        } else {
                            spdlog::error("Failed to connect to port {}",
                                _slowdaq_doe.PFEIFFERPort);

                            _slowdaq_doe.PFEIFFERState
                                = PFEIFFERSSGState::Standby;
                        }
                    } else {
                        // No need for a message
                        _slowdaq_doe.PFEIFFERState
                            = PFEIFFERSSGState::Standby;
                    }
                }
                break;


                case PFEIFFERSSGState::Connected:
                    PFEIFFER_update();
                break;

                case PFEIFFERSSGState::Disconnected:
                    _logger->warn("Manually losing connection to "
                        "PFEIFFER Pressure valve with port {}",
                        _slowdaq_doe.PFEIFFERPort);
                    disconnect(_pfeiffers_port);
                                            // Move to standby
                    _slowdaq_doe.PFEIFFERState
                        = PFEIFFERSSGState::Standby;
                break;

                case PFEIFFERSSGState::Closing:
                    _logger->info("Going to close the slow DAQ thread.");
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

                default:
                    _slowdaq_doe.PFEIFFERState = PFEIFFERSSGState::Standby;
            }

            return true;
        };

        // This will call rlf_block_time every 1 ms
        // microseconds to have better resolution or less error perhaps
        auto main_loop_block_time = make_blocking_total_timed_event(
            std::chrono::microseconds(1000), main_loop);

        // Actual loop!
        while (main_loop_block_time());
    }

    void PFEIFFER_update() {
        static auto retrieve_time = _slowdaq_doe.PFEIFFERSingleGaugeUpdateSpeed ==
        PFEIFFERSingleGaugeSP::SLOW ? 60*1000 : _slowdaq_doe.PFEIFFERSingleGaugeUpdateSpeed ==
        PFEIFFERSingleGaugeSP::FAST ? 1000 : 100;

        static auto retrieve_press_nb = make_total_timed_event(
            std::chrono::milliseconds(retrieve_time),
            // Lambda hacking to allow the class function to be pass to
            // make_total_timed_event. Is there any other way?
            [&]() {
                auto msg = retrieve_msg<std::string>(_pfeiffers_port);

                if (not msg.has_value()) {
                    return;
                }
                if (msg.value().empty()) {
                    return;
                }

                auto split_msg = split(msg.value(), ",");

                if (split_msg.size() <= 1) {
                    return;
                }

                double pressure = std::stod(split_msg[1]);

                _slowdaq_doe.Vacuum = pressure;
                _slowdaq_doe.PressureData(get_current_time_epoch()/1000.0, pressure);

                PFEIFFERSingleGaugeData d;
                d.Pressure = pressure;
                d.time = get_current_time_epoch();
                _pfeiffer_file->add(d);
            });

        static auto save_files = make_total_timed_event(
            std::chrono::seconds(30),
            [&](){
                _logger->info("Saving PFEIFFER data...");

                _pfeiffer_file->async_save([](const PFEIFFERSingleGaugeData& data) {
                    std::ostringstream out;
                    out.precision(4);
                    out << std::scientific;
                    out << data.Pressure;
                    return std::to_string(data.time) + "," + out.str() + "\n";
                });
        });

        retrieve_press_nb();
        save_files();
    }

    // https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
    std::vector<std::string> split(std::string s, std::string delimiter) {
        size_t pos_start = 0, pos_end, delim_len = delimiter.length();
        std::string token;
        std::vector<std::string> res;

        while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
            token = s.substr(pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            res.push_back(token);
        }

        res.push_back(s.substr(pos_start));
        return res;
    }
};

template<typename Pipes>
auto make_slow_daq_manager(const Pipes& p) {
    return std::make_unique<SlowDAQManager<Pipes>>(p);
}

}  // namespace SBCQueens
#endif
