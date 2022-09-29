#ifndef CAENDIGITIZERINTERFACE_H
#define CAENDIGITIZERINTERFACE_H
#pragma once

// C 3rd party includes
// C++ std includes
#include <array>
#include <vector>
#include <tuple>
#include <cstdint>
#include <functional>
#include <memory>
#include <numeric>
#include <string>
#include <thread>
#include <chrono>
#include <random>
#include <iostream>

// C++ 3rd party includes
#include <serial/serial.h>
#include <readerwriterqueue.h>
#include <spdlog/spdlog.h>

// my includes
#include "include/serial_helper.hpp"
#include "include/file_helpers.hpp"
#include "include/caen_helper.hpp"
#include "include/implot_helpers.hpp"
#include "include/indicators.hpp"
#include "include/timing_events.hpp"
#include "include/HardwareHelpers/ClientController.hpp"

namespace SBCQueens {

struct Keithley2000Measure {
    double Volt;
    double Time;  // in unix timestamp
};

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

struct CAENInterfaceData {
    std::string RunDir = "";
    std::string RunName =  "";
    std::string SiPMParameters = "";

    CAENConnectionType ConnectionType;

    CAENDigitizerModel Model;
    CAENGlobalConfig GlobalConfig;
    std::vector<CAENGroupConfig> GroupConfigs;

    int PortNum = 0;
    uint32_t VMEAddress = 0;

    CAENInterfaceStates CurrentState = CAENInterfaceStates::NullState;

    bool SoftwareTrigger = false;

    std::string Keithley2000Port = "";
};

using CAENQueueType = std::function < bool(CAENInterfaceData&) >;
using CAENQueue = moodycamel::ReaderWriterQueue< CAENQueueType >;

template<typename... Queues>
class CAENDigitizerInterface {
    // Software related items
    std::tuple<Queues&...> _queues;
    CAENInterfaceData doe;
    IndicatorSender<IndicatorNames> _indicatorSender;
    IndicatorSender<uint8_t> _plotSender;

    // Files
    DataFile<CAENEvent> _pulseFile;
    DataFile<Keithley2000Measure> _voltagesFile;

    // Hardware
    CAEN Port = nullptr;
    ClientController<serial_ptr, double> Keithley2000;

    Keithley2000Measure latest_measure;
    std::uint64_t SavedWaveforms = 0;

    // tmp stuff
    uint16_t* data;
    size_t length;
    std::vector<double> x_values, y_values;
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
        _indicatorSender(std::get<GeneralIndicatorQueue&>(_queues)),
        _plotSender(std::get<SiPMPlotQueue&>(_queues)),
        Keithley2000("Keithley2000") {
        // This is possible because std::function can be assigned
        // to whatever std::bind returns
        standby_state = std::make_shared<CAENInterfaceState>(
            std::chrono::milliseconds(100),
            std::bind(&CAENDigitizerInterface::standby, this));

        attemptConnection_state = std::make_shared<CAENInterfaceState>(
            std::chrono::milliseconds(1),
            std::bind(&CAENDigitizerInterface::attempt_connection, this));

        oscilloscopeMode_state = std::make_shared<CAENInterfaceState>(
            std::chrono::milliseconds(100),
            std::bind(&CAENDigitizerInterface::oscilloscope, this));

        statisticsMode_state = std::make_shared<CAENInterfaceState>(
            std::chrono::milliseconds(1),
            std::bind(&CAENDigitizerInterface::statistics_mode, this));

        runMode_state = std::make_shared<CAENInterfaceState>(
            std::chrono::milliseconds(1),
            std::bind(&CAENDigitizerInterface::run_mode, this));

        disconnected_state = std::make_shared<CAENInterfaceState>(
            std::chrono::milliseconds(1),
            std::bind(&CAENDigitizerInterface::disconnected_mode, this));

        // auto g =
        closing_state = std::make_shared<CAENInterfaceState>(
            std::chrono::milliseconds(1),
            std::bind(&CAENDigitizerInterface::closing_mode, this));
    }

    // No copying
    CAENDigitizerInterface(const CAENDigitizerInterface&) = delete;

    ~CAENDigitizerInterface() {}

    void operator()() {
        spdlog::info("Initializing CAEN thread");

        main_loop_state = standby_state;

        Keithley2000.init(
            [=](serial_ptr& port) -> bool {

                static auto send_msg_slow = make_blocking_total_timed_event(
                    std::chrono::milliseconds(200),
                    [](serial_ptr& port, const std::string& msg){
                        spdlog::info("Sending msg to Keithely: {0}", msg);
                        send_msg(port, msg + "\n", "");
                    }
                );

                SerialParams ssp;
                ssp.Timeout = serial::Timeout::simpleTimeout(5000);
                ssp.Baudrate = 115200;
                connect_par(port, doe.Keithley2000Port, ssp);

                if (!port) {
                    return false;
                }

                if (!port->isOpen()) {
                    return false;
                }

                spdlog::info("Connected to Keithley 2000! "
                    "Initializing...");

                // Setup GPIB interface
                // send_msg_slow(port, "++rst");
                send_msg_slow(port, "++eos 0");
                send_msg_slow(port, "++addr 10");
                send_msg_slow(port, "++auto 0");

                // Now Keithley
                send_msg_slow(port, "*rst");
                send_msg_slow(port, ":init:cont on");
                send_msg_slow(port, ":volt:dc:nplc 10");
                send_msg_slow(port, ":volt:dc:rang:auto 1");
                send_msg_slow(port, ":volt:dc:aver:stat 1");
                send_msg_slow(port, ":volt:dc:aver:tcon mov");
                send_msg_slow(port, ":volt:dc:aver:coun 100");

                send_msg_slow(port, ":form ascii");

                send_msg_slow(port, "++auto 1");

                open(_voltagesFile, doe.RunDir
                    + "/" + doe.RunName
                    + "/SIPM_VOLTAGES.txt");

                bool s = _voltagesFile > 0;
                if (!s) {
                    spdlog::info("Failed to open voltage file");
                    return false;
                }

                return true;
            },
            [=](serial_ptr& port) -> bool {

                if(port) {
                    if(port->isOpen()) {
                        disconnect(port);
                    }
                }

                return false;
        });

        // Actual loop!
        while (main_loop_state->operator()()) {

            static auto keith = make_total_timed_event(
                std::chrono::seconds(5),
                [&]() {
                    auto r = Keithley2000.get([=](serial_ptr& port)
                    -> std::optional<double> {
                    send_msg(port, "++addr 10\n", "");
                    send_msg(port, ":fetch?\n", "");

                    return retrieve_msg<double>(port);
                });

                if (r) {
                    double time = get_current_time_epoch() / 1000.0;
                    double volt = r.value();

                    latest_measure.Volt = volt;
                    latest_measure.Time = time;
                    _voltagesFile->Add(latest_measure);
                    _indicatorSender(IndicatorNames::DMM_VOLTAGE, time, volt);
                    _indicatorSender(IndicatorNames::LATEST_DMM_VOLTAGE, volt);
                }
            });

            static auto save_keith = make_total_timed_event(
                std::chrono::seconds(30),
                [&]() {
                    spdlog::info("Saving voltage (Keithley 2000) voltages.");
                    async_save(_voltagesFile,
                    [](const Keithley2000Measure& mes) {
                            return  std::to_string(mes.Time) + "," +
                                    std::to_string(mes.Volt) + "\n";
                });

            });

            switch (doe.CurrentState) {
                case CAENInterfaceStates::OscilloscopeMode:
                case CAENInterfaceStates::StatisticsMode:
                case CAENInterfaceStates::RunMode:
                    keith();
                    save_keith();
                break;

                case CAENInterfaceStates::Standby:
                case CAENInterfaceStates::AttemptConnection:
                case CAENInterfaceStates::Disconnected:
                case CAENInterfaceStates::Closing:
                case CAENInterfaceStates::NullState:
                default:
                break;
            }


        }
    }

 private:
    double extract_frequency(CAENEvent& evt) {
        auto buf = evt->Data->DataChannel[0];
        auto size = evt->Data->ChSize[0];

        double counter = 0;
        auto offset = Port->GroupConfigs[0].TriggerThreshold;
        for (uint32_t i = 1; i < size; i++) {
            if ((buf[i] > offset) && (buf[i-1] < offset)) {
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
        doe.CurrentState = newState;
        switch (doe.CurrentState) {
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
        if (guiQueueOut.try_dequeue(task)) {
            if (!task(doe)) {
                spdlog::warn("Something went wrong with a command!");
            } else {
                switch_state(doe.CurrentState);
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
        auto err = connect(Port,
            doe.Model,
            doe.ConnectionType,
            doe.PortNum,
            0,
            doe.VMEAddress);

        // If port resource was not created, it equals a failure!
        if (!Port) {
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
            doe.GlobalConfig,
            doe.GroupConfigs);

        enable_acquisition(Port);

        // Getting at least one initializes the port
        Keithley2000.get([=](serial_ptr& port) -> std::optional<double> {
            return {};
        });


        // Allocate memory for events
        osc_event = std::make_shared<caenEvent>(Port->Handle);

        for (CAENEvent& evt : processing_evts) {
            evt = std::make_shared<caenEvent>(Port->Handle);
        }

        auto failed = lec();

        if (failed) {
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
        _indicatorSender(IndicatorNames::CAENBUFFEREVENTS, events);

        retrieve_data(Port);

        if (doe.SoftwareTrigger) {
            software_trigger(Port);
            doe.SoftwareTrigger = false;
        }

        if (Port->Data.DataSize > 0 && Port->Data.NumEvents > 0) {
            spdlog::info("Total size buffer: {0}",  Port->Data.TotalSizeBuffer);
            spdlog::info("Data size: {0}", Port->Data.DataSize);
            spdlog::info("Num events: {0}", Port->Data.NumEvents);

            extract_event(Port, 0, osc_event);
            spdlog::info("Event size: {0}", osc_event->Info.EventSize);
            spdlog::info("Event counter: {0}", osc_event->Info.EventCounter);
            spdlog::info("Trigger Time Tag: {0}",
                osc_event->Info.TriggerTimeTag);

            process_data_for_gui();

            // if(port->Data.NumEvents >= 2) {
            //  extract_event(port, 1, adj_osc_event);

            //  auto t0 = osc_event->Info.TriggerTimeTag;
            //  auto t1 = adj_osc_event->Info.TriggerTimeTag;

            //  spdlog::info("Time difference between events: {0}", t1 - t0);
            // }

            // Clear events in buffer
            clear_data(Port);
        }


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
                Port->GlobalConfig.MaxEventsPerRead);

            // While all of this is happening, the digitizer is taking data
            if (!isData) {
                return;
            }

            // double frequency = 0.0;
            for (uint32_t i = 0; i < Port->Data.NumEvents; i++) {
                // Extract event i
                extract_event(Port, i, processing_evts[i]);
            }
        };

        static auto extract_for_gui_nb = make_total_timed_event(
            std::chrono::milliseconds(200),
            std::bind(&CAENDigitizerInterface::rdm_extract_for_gui, this));

        static auto checkerror = make_total_timed_event(
            std::chrono::seconds(1),
            std::bind(&CAENDigitizerInterface::lec, this));


        process_events();
        checkerror();
        extract_for_gui_nb();
        // extract_for_gui_nb();
        change_state();
        return true;
    }

    bool run_mode() {
        static bool isFileOpen = false;
        static auto extract_for_gui_nb = make_total_timed_event(
            std::chrono::milliseconds(200),
            std::bind(&CAENDigitizerInterface::rdm_extract_for_gui, this));
        static auto process_events = [&]() {
            bool isData = retrieve_data_until_n_events(Port,
                Port->GlobalConfig.MaxEventsPerRead);

            // While all of this is happening, the digitizer is taking data
            if (!isData) {
                return;
            }

            // double frequency = 0.0;
            for (uint32_t i = 0; i < Port->Data.NumEvents; i++) {
                // Extract event i
                extract_event(Port, i, processing_evts[i]);

                if (_pulseFile) {
                    // Copy event to the file buffer
                    _pulseFile->Add(processing_evts[i]);
                }
            }

            SavedWaveforms += Port->Data.NumEvents;
            _indicatorSender(IndicatorNames::SAVED_WAVEFORMS, SavedWaveforms);
        };

        if (isFileOpen) {
            // spdlog::info("Saving SIPM data");
            save(_pulseFile, sbc_save_func, Port);
        } else {
            // Get current date and time, e.g. 202201051103
            std::chrono::system_clock::time_point now
                = std::chrono::system_clock::now();
            std::time_t now_t = std::chrono::system_clock::to_time_t(now);
            char filename[16];
            std::strftime(filename, sizeof(filename), "%Y%m%d%H%M",
                std::localtime(&now_t));

            open(_pulseFile,
                doe.RunDir
                + "/" + doe.RunName
                + "/" + filename + ".bin",
                // + doe.SiPMParameters + ".bin",

                // sbc_init_file is a function that saves the header
                // of the sbc data format as a function of record length
                // and number of channels
                sbc_init_file,
                Port);

            isFileOpen = _pulseFile > 0;
            SavedWaveforms = 0;
        }

        process_events();
        extract_for_gui_nb();
        if (change_state()) {
            // save remaining data
            retrieve_data(Port);
            if (isFileOpen) {
                for (uint32_t i = 0; i < Port->Data.NumEvents; i++) {
                    extract_event(Port, i, processing_evts[i]);
                    _pulseFile->Add(processing_evts[i]);
                }
                save(_pulseFile, sbc_save_func, Port);
            }

            isFileOpen = false;
            close(_pulseFile);
        }
        return true;
    }

    bool disconnected_mode() {
        spdlog::warn("Manually losing connection to the "
                        "CAEN digitizer.");

        for (CAENEvent& evt : processing_evts) {
            evt.reset();
        }

        osc_event.reset();
        adj_osc_event.reset();
        Keithley2000.close();

        auto err = disconnect(Port);
        check_error(err, [](const std::string& cmd) {
            spdlog::error(cmd);
        });

        switch_state(CAENInterfaceStates::Standby);
        return true;
    }

    bool closing_mode() {
        spdlog::info("Going to close the CAEN thread.");
        for (CAENEvent& evt : processing_evts) {
            evt.reset();
        }

        osc_event.reset();
        adj_osc_event.reset();
        Keithley2000.close();

        auto err = disconnect(Port);
        check_error(err, [](const std::string& cmd) {
            spdlog::error(cmd);
        });

        return false;
    }

    void rdm_extract_for_gui() {
        if (Port->Data.DataSize <= 0) {
            return;
        }

        // For the GUI
        std::default_random_engine generator;
        std::uniform_int_distribution<uint32_t>
            distribution(0, Port->Data.NumEvents);
        uint32_t rdm_num = distribution(generator);

        extract_event(Port, rdm_num, osc_event);

        process_data_for_gui();
    }

    void process_data_for_gui() {
        // int j = 0;

        // TODO(Zhiheng): change anything you need here, too...
        // for(auto ch : Port->GroupConfigs) {
        for (std::size_t j = 0; j < Port->GetNumChannels(); j++) {
            // auto& ch_num = ch.second.Number;
            auto buf = osc_event->Data->DataChannel[j];
            auto size = osc_event->Data->ChSize[j];

            if (size <= 0) {
                continue;
            }

            x_values.resize(size);
            y_values.resize(size);

            for (std::size_t i = 0; i < size; i++) {
                x_values[i] = i*(1.0/Port->GetSampleRate())*(1e9);
                y_values[i] = static_cast<double>(buf[i]);
            }

            _plotSender(static_cast<uint8_t>(j),
                x_values,
                y_values);
        }
    }
};
}  // namespace SBCQueens
#endif
