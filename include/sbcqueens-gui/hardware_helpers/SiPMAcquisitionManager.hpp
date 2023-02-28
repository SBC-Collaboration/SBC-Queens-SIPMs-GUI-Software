#ifndef SIPMACQUISITIONMANAGER_H
#define SIPMACQUISITIONMANAGER_H
#include <future>
#include <optional>
#pragma once

// C STD includes
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
#include <algorithm>
#include <filesystem>

// C++ 3rd party includes
#include <date/date.h>
#include <armadillo>
#include <serial/serial.h>
#include <readerwriterqueue.h>
#include <spdlog/spdlog.h>

// my includes
#include "sbcqueens-gui/multithreading_helpers/ThreadManager.hpp"

#include "sbcqueens-gui/serial_helper.hpp"
#include "sbcqueens-gui/file_helpers.hpp"
#include "sbcqueens-gui/caen_helper.hpp"
#include "sbcqueens-gui/implot_helpers.hpp"
#include "sbcqueens-gui/timing_events.hpp"
#include "sbcqueens-gui/armadillo_helpers.hpp"

#include "sbcqueens-gui/hardware_helpers/SiPMAcquisitionData.hpp"
#include "sbcqueens-gui/hardware_helpers/ClientController.hpp"
#include "sbcqueens-gui/hardware_helpers/Calibration.hpp"

#include "sbcqueens-gui/sipm_helpers/SBCBinaryFormat.hpp"

// #include "sbcqueens-gui/sipm_helpers/BreakDownRoutine.hpp"
// #include "sbcqueens-gui/sipm_helpers/AcquisitionRoutine.hpp"

#include "sipmanalysis/PulseFunctions.hpp"
#include "sipmanalysis/SPEAnalysis.hpp"
#include "sipmanalysis/GainVBDEstimation.hpp"

namespace SBCQueens {

template<class Pipes>
class SiPMAcquisitionManager : public ThreadManager<Pipes> {
    // To get the pipe interface used in the pipes.
    using SiPMPipe_type = typename Pipes::SiPMPipe_type;
    // Software related items
    SiPMAcquisitionPipeEnd<SiPMPipe_type, PipeEndType::Consumer> _sipm_pipe_end;
    // A reference to the DOE thats inside _caen_pipe_end
    SiPMAcquisitionData& _doe;

    std::shared_ptr<spdlog::logger> _logger;

    using SiPMCAEN = CAEN<std::shared_ptr<spdlog::logger>>;
    using SiPMCAEN_ptr = std::unique_ptr<SiPMCAEN>;

    using SiPMCAENFile_ptr = std::unique_ptr<BinaryFormat::SiPMDynamicWriter>;
    SiPMCAENFile_ptr _caen_file = nullptr;

    using SiPMWaveforms_ptr = std::shared_ptr<CAENWaveforms<uint16_t>>;
    std::vector<SiPMWaveforms_ptr> _waveforms;

    // Files
    std::string _run_name;
    DataFile<SiPMVoltageMeasure> _voltages_file;
    std::unique_ptr<LogFile> _saveinfo_file = nullptr;

    // Hardware
    uint8_t _num_chs = 0;
    double _acq_rate = 0.0;

    uint32_t SavedWaveforms = 0;
    uint64_t TriggeredWaveforms = 0;

    bool _vbd_created = false;

    // tmp stuff
    double _wait_time = 900000.0;
    double _reset_timer = 0;
    uint16_t* _data = nullptr;
    size_t _length = 0;
    const CAENEvent* _osc_event = nullptr;
    // 1024 because the caen can only hold 1024 no matter what model (so far)
    std::array<const CAENEvent*, 1024> _processing_evts = {nullptr};

    // Analysis
    // std::unique_ptr<BreakdownRoutine> _vbd_routine = nullptr;
    // GainVBDFitParameters _latest_breakdown_voltage;
    // std::unique_ptr<AcquisitionRoutine> _acq_routine = nullptr;

    // As long as we make the Func template argument a std::fuction
    // we can then use pointer magic to initialize them
    using SiPMAcquisitioneState
        = BlockingTotalTimeEvent<
            std::function<bool(void)>,
            std::chrono::milliseconds
        >;

    // We need to turn the class BlockingTotalTimeEvent into a pointer
    // because it does not have an empty constructor and for a good reason
    // if its empty, bugs and crashes would happen all the time.
    using SiPMAcquisitionState_ptr = std::shared_ptr<SiPMAcquisitioneState>;

    SiPMAcquisitionState_ptr main_loop_state;

    SiPMAcquisitionState_ptr standby_state;
    SiPMAcquisitionState_ptr acquisition_state;
    SiPMAcquisitionState_ptr closing_state;

 public:
    explicit SiPMAcquisitionManager(const Pipes& pipes) :
        ThreadManager<Pipes>(pipes),
        _sipm_pipe_end(pipes.SiPMPipe), _doe{_sipm_pipe_end.Data} {
        // This is possible because std::function can be assigned
        // to whatever std::bind returns
        standby_state = std::make_shared<SiPMAcquisitioneState>(
            std::chrono::milliseconds(1000),
            std::bind(&SiPMAcquisitionManager::standby, this));

        acquisition_state = std::make_shared<SiPMAcquisitioneState>(
            std::chrono::milliseconds(1),
            std::bind(&SiPMAcquisitionManager::acquisition, this));

        closing_state = std::make_shared<SiPMAcquisitioneState>(
            std::chrono::milliseconds(1),
            std::bind(&SiPMAcquisitionManager::closing_mode, this));

        _logger = spdlog::get("log");
    }

    ~SiPMAcquisitionManager() {
        _logger->info("Closing SiPM Acquisition Manager");
    }

    void operator()() {
        _logger->info("Initializing CAEN thread");

        _doe.IVData = PlotDataBuffer<2>(100);

        main_loop_state = standby_state;

        // _sipm_volt_sys.build(
        //     [=] (serial_ptr& port) -> bool {
        //         static auto send_msg_slow = make_blocking_total_timed_event(
        //             std::chrono::milliseconds(200),
        //             [&](const std::string& msg){
        //                 spdlog::info("Sending msg to Keithely: {0}", msg);
        //                 send_msg(port, msg + "\n", "");
        //             }
        //         );

        //         SerialParams ssp;
        //         ssp.Timeout = serial::Timeout::simpleTimeout(5000);
        //         ssp.Baudrate = 115200;
        //         connect_par(port, _doe.SiPMVoltageSysPort, ssp);

        //         if (!port) {
        //             spdlog::error("Port with port name {0} was not open",
        //                 _doe.SiPMVoltageSysPort);
        //             return false;
        //         }

        //         if (!port->isOpen()) {
        //             spdlog::error("Port with port name {0} was not open",
        //                 _doe.SiPMVoltageSysPort);
        //             return false;
        //         }

        //         spdlog::info("Connected to Keithley 2000! "
        //             "Initializing...");

        //         // Setup GPIB interface
        //         // send_msg_slow("++rst");
        //         // This should be not run if we are in debug mode.
        //     #ifdef NDEBUG
        //         send_msg_slow("++eos 0");
        //         send_msg_slow("++addr 10");
        //         send_msg_slow("++auto 0");

        //         //// if tired of waiting for keithley comment starting here:
        //         // Now Keithley
        //         // These parameters are constant for now
        //         // but later they can become a parameter or the voltage
        //         // system can be optional at all
        //         send_msg_slow("*rst");
        //         send_msg_slow(":init:cont on");
        //         send_msg_slow(":volt:dc:nplc 10");
        //         send_msg_slow(":volt:dc:rang:auto 0");
        //         send_msg_slow(":volt:dc:rang 100");
        //         send_msg_slow(":volt:dc:aver:stat 0");
        //         // send_msg_slow(":volt:dc:aver:tcon mov");
        //         // send_msg_slow(":volt:dc:aver:coun 100");

        //         send_msg_slow(":form ascii");

        //         // Keithley 6487
        //         send_msg_slow("++addr 22");
        //         send_msg_slow("*rst");
        //         send_msg_slow(":form:elem read");

        //         // Volt supply side
        //         send_msg_slow(":sour:volt:rang 55");
        //         send_msg_slow(":sour:volt:ilim 250e-6");
        //         send_msg_slow(":sour:volt "
        //             + std::to_string(_doe.SiPMVoltageSysVoltage));
        //         send_msg_slow(":sour:volt:stat OFF");

        //         // Ammeter side
        //         send_msg_slow(":sens:curr:dc:nplc 6");
        //         send_msg_slow(":sens:aver:stat off");

        //         // Zero check
        //         send_msg_slow(":syst:zch ON");
        //         send_msg_slow(":curr:rang 2E-7");
        //         send_msg_slow.ChangeWaitTime(std::chrono::milliseconds(1000));
        //         send_msg_slow(":init");
        //         send_msg_slow(":syst:zcor:stat OFF");
        //         send_msg_slow(":syst:zcor:acq");
        //         send_msg_slow(":syst:zch OFF");
        //         send_msg_slow(":syst:zcor ON");
        //         send_msg_slow(":syst:azer ON");

        //         send_msg_slow.ChangeWaitTime(std::chrono::milliseconds(200));
        //         send_msg_slow(":arm:coun 1");
        //         send_msg_slow(":arm:sour imm");
        //         send_msg_slow(":arm:timer 0");
        //         send_msg_slow(":trig:coun 1");
        //         send_msg_slow(":trig:sour imm");

        //         send_msg_slow.ChangeWaitTime(std::chrono::milliseconds(1100));
        //         send_msg_slow(":init");
        //     #endif

        //         return true;
        //     },
        //     [=](serial_ptr& port) -> bool {
        //         if (port) {
        //             send_msg(port, "++addr 22\n", "");
        //             send_msg(port, ":sour:volt:stat OFF\n", "");
        //             send_msg(port, ":sour:volt 0.0\n", "");

        //             if (port->isOpen()) {
        //                 disconnect(port);
        //             }
        //         }

        //         return false;
        // });

        // Actual loop!
        while (main_loop_state->operator()()) {
            // sipm_voltage_system_update();
        }
    }

 private:
    void calculate_trigger_frequency() {
        static double last_time = get_current_time_epoch() / 1000.0;
        static uint64_t last_waveforms = 0;
        double current_time = get_current_time_epoch() / 1000.0;
        double dt = current_time - last_time;
        uint64_t dWaveforms = TriggeredWaveforms - last_waveforms;

        last_waveforms = TriggeredWaveforms;
        last_time = current_time;
        _doe.TriggeredRate = static_cast<double>(dWaveforms) / dt;
    }

    // Changes the manager state. Currently only 3:
    // Acquisition, Closing, and Standby
    void switch_state(const SiPMAcquisitionManagerStates& newState) {
        _doe.CurrentState = newState;
        switch (_doe.CurrentState) {
            case SiPMAcquisitionManagerStates::Acquisition:
                main_loop_state = acquisition_state;
            break;

            case SiPMAcquisitionManagerStates::Closing:
                main_loop_state = closing_state;
            break;

            case SiPMAcquisitionManagerStates::Standby:
            default:
                // do nothing other than set to standby state
                main_loop_state = standby_state;
        }
    }

    // Envelopes the logic that listens to an external queue
    // that can the data inside this thread.
    void change_state() {
        // GUI -> CAEN
        SiPMAcquisitionData task;

        // If the queue does not return a valid callback, this call will
        // do nothing and should return true always.
        // The tasks are essentially any GUI driven modification, example
        // setting the PID setpoints or constants
        // or an user driven reset
        if (_sipm_pipe_end.retrieve(task)) {
            task.Callback(_doe);
            switch_state(_doe.CurrentState);
        }

        static auto send_data_tt = make_total_timed_event(
                std::chrono::milliseconds(200),
                [&]() {
                    _sipm_pipe_end.send();
                }
        );
        // Send the current state to the GUI to update
        send_data_tt();
    }

    // Does nothing other than wait 1000ms to avoid clogging PC resources.
    bool standby() {
        change_state();
        return true;
    }

    bool acquisition() {
        auto caen_res = attempt_connection();
        if (caen_res->HasError()) {
            caen_res.reset();
            change_state();
            return true;
        }

        // We are stuck inside this while loop which will break under
        // three conditions:
        // 1. There is a fatal error in the CAEN digitizer.
        // 2. GUI changes the state of the manager to something different
        //     other than acquisition mode.
        // 3.- Any other condition under acquisition mode. Such as number of
        //     samples but it can always ran to go forever
        while (not caen_res->HasError()) {
            switch(_doe.AcquisitionState) {
                case SiPMAcquisitionStates::Oscilloscope:
                    if(_caen_file) {
                        _caen_file.reset();
                    }

                    caen_res = oscilloscope(std::move(caen_res));
                    break;

                case SiPMAcquisitionStates::EndlessAcquisition:
                    caen_res = acquisition_endless(std::move(caen_res));
                    break;

                case SiPMAcquisitionStates::NumberedAcquisition:
                    break;

                // Resets the setup information without freeing the CAEN resource
                case SiPMAcquisitionStates::Reset:
                    if(_caen_file) {
                        _caen_file.reset();
                    }
                    caen_res = setup_and_prepare(std::move(caen_res));
                    break;
            }

            // This manages the communication with the GUI
            change_state();
            // If the GUI says, hey stop acquiring, we do!
            if (_doe.CurrentState != SiPMAcquisitionManagerStates::Acquisition) {
                break;
            }
        }

        // Once we go out of scope, we release/disconnect the CAEN
        caen_res.reset();
        _caen_file.reset();
        return true;
    }

    // Attempts a connection to the CAEN digitizer, setups the channels,
    // starts acquisition, and moves to the oscilloscope mode
    SiPMCAEN_ptr attempt_connection() {
        auto caen_port = std::make_unique<SiPMCAEN>(_logger,
                                  _doe.Model,
                                  _doe.ConnectionType,
                                  _doe.PortNum,
                                  0,
                                  _doe.VMEAddress);

        // If port resource was not created, it equals a failure!
        if (not caen_port->IsConnected()) {
            switch_state(SiPMAcquisitionManagerStates::Standby);
            return caen_port;
        }

        return setup_and_prepare(std::move(caen_port));
    }

    SiPMCAEN_ptr setup_and_prepare(SiPMCAEN_ptr caen_port) {
        caen_port->Setup(_doe.GlobalConfig, _doe.GroupConfigs);
        if(caen_port->HasError()) {
            switch_state(SiPMAcquisitionManagerStates::Standby);
            return caen_port;
        }

        // The setup functions does change and make calculations
        // about some parameters we pass, we read them back to get a
        // more accurate value of them.
        _doe.GlobalConfig = caen_port->GetGlobalConfiguration();
        _doe.GroupConfigs = caen_port->GetGroupConfigurations();

        // Initialize the plotting data
        std::generate(_doe.GroupData.begin(), _doe.GroupData.end(), [&](){
            return PlotDataBuffer<8>(_doe.GlobalConfig.RecordLength);
        });

        // Fill them up as by default they act like circular buffers
        for(auto& data : _doe.GroupData) {
            data.fill();
        }

        _num_chs = caen_port->ModelConstants.NumChannels;
        _acq_rate = caen_port->ModelConstants.AcquisitionRate;
        _doe.CAENBoardInfo = caen_port->GetBoardInfo();

        // Enable acquisition HAS to be called AFTER setup
        caen_port->EnableAcquisition();
        if(caen_port->HasError()) {
            switch_state(SiPMAcquisitionManagerStates::Standby);
            return caen_port;
        }

        // Given its a pointer _osc_event will always point to the first
        // even in the buffer
        _osc_event = caen_port->GetEvent(0);

        for(std::size_t i = 0; i < caen_port->GetCurrentPossibleMaxBuffer(); i++) {
            _waveforms.push_back(caen_port->GetWaveform(i));
        }

        // These lines get today's date and creates a folder under that date
        // There is a similar code in the Teensy interface file
        std::ostringstream out;
        auto today = date::year_month_day{
                date::floor<date::days>(std::chrono::system_clock::now())};
        out << today;
        _run_name = out.str();

        std::filesystem::create_directory(_doe.RunDir
                                          + "/" + _run_name);

        _logger->info("CAEN Setup complete!");
        _doe.AcquisitionState = SiPMAcquisitionStates::Oscilloscope;
        return caen_port;
    }

    // While in this state it shares the data with the GUI but
    // no actual file saving is happening. It essentially serves
    // as a mode in where the user can see what is happening.
    // Similar to an oscilloscope
    SiPMCAEN_ptr oscilloscope(SiPMCAEN_ptr caen_port) {
        auto n_events = caen_port->GetEventsInBuffer();
        // _indicator_sender(IndicatorNames::CAENBUFFEREVENTS, events);

        software_trigger(caen_port);

//        static BinaryFormat::SiPMDynamicWriter _file("sipm_waveforms.bin",
//                                       caen_port->ModelConstants,
//                                       caen_port->GetGlobalConfiguration(),
//                                       caen_port->GetGroupConfigurations());

        caen_port->RetrieveData();
        // GetNumberOfEvents gets the actual acquired events
        // while GetEventsInBuffer gets the events in CAEN buffer
        if (caen_port->GetNumberOfEvents() > 0) {
            // TriggeredWaveforms += _caen_port->Data.NumEvents;
            // Due to the slow rate oscilloscope mode is happening, using
            // events instead of ..->Data.NumEvents is a better number to use
            // to approximate the actual number of waveforms acquired.
            // Specially because the buffer is cleared.
            TriggeredWaveforms += n_events;
            // spdlog::info("Total size buffer: {0}",  _caen_port->Data.TotalSizeBuffer);
            // spdlog::info("Data size: {0}", _caen_port->Data.DataSize);
            // spdlog::info("Num events: {0}", _caen_port->Data.NumEvents);

            // Decode events
            caen_port->DecodeEvents();

            auto m = caen_event_to_armadillo(_osc_event, 64);

            auto waveform = caen_port->GetWaveform(0);
//            _file.save_waveform(waveform);

            // spdlog::info("Event size: {0}", _osc_event->Info.EventSize);
            // spdlog::info("Event counter: {0}", _osc_event->Info.EventCounter);
            // spdlog::info("Trigger Time Tag: {0}",
            //     _osc_event->Info.TriggerTimeTag);

            process_data_for_gui();

            // Clear events in buffer
            caen_port->ClearData();
        }

        return caen_port;
    }

    SiPMCAEN_ptr acquisition_endless(SiPMCAEN_ptr caen_port) {
        if(not _caen_file) {
            try {
                _caen_file = std::make_unique<BinaryFormat::SiPMDynamicWriter>(
                        _doe.RunDir + "/" + _run_name + "/" + _doe.SiPMOutputName + ".bin",
                        caen_port->ModelConstants,
                        caen_port->GetGlobalConfiguration(),
                        caen_port->GetGroupConfigurations());
            } catch(std::runtime_error err) {
                if (not _caen_file->isOpen()) {
                    _logger->error("SiPM file saving was not created with error: {}",
                                   err.what());
                    _doe.AcquisitionState = SiPMAcquisitionStates::Oscilloscope;
                    return caen_port;
                }
            }
        }

        software_trigger(caen_port);

        auto n_events = caen_port->GetEventsInBuffer();

        if (caen_port->RetrieveDataUntilNEvents(0.5*caen_port->GetCurrentPossibleMaxBuffer())) {
            TriggeredWaveforms += n_events;

            // This should update the values under _waveforms
            caen_port->DecodeEvents();

            // TODO: here be the filtering/software threshold routine

            std::for_each_n(_waveforms.begin(),
                            caen_port->GetNumberOfEvents(),
                            [&](SiPMWaveforms_ptr& waveform) {
                                _caen_file->save_waveform(waveform);
                            }
            );
//JVW5K
            process_data_for_gui();
        }

        return caen_port;
    }

    void software_trigger(SiPMCAEN_ptr& caen_port) {
        if (_doe.SoftwareTrigger) {
            _logger->info("Sending a software trigger");
            caen_port->SoftwareTrigger();
            _doe.SoftwareTrigger = false;
        }
    }

    // Only mode that stops the main_loop and frees all resources
    bool closing_mode() {
        _logger->info("Going to close the CAEN thread.");
        return false;
    }

    void rdm_extract_for_gui() {
        static auto rdm_extract_timed = make_total_timed_event(
            std::chrono::milliseconds(200),
            [&]() {
                // For the GUI
                if (TriggeredWaveforms == 0) {
                    return;
                }
                static std::default_random_engine generator;
                std::uniform_int_distribution<uint64_t>
                    distribution(0, TriggeredWaveforms);
                uint64_t rdm_num = distribution(generator);

                _osc_event = _processing_evts[rdm_num];

                process_data_for_gui();
        });

        rdm_extract_timed();
    }

    void process_data_for_gui() {
        calculate_trigger_frequency();

        if (not _osc_event) {
            return;
        }

        auto size = _osc_event->getData()->ChSize[0];
        auto all_chs = _osc_event->getData()->DataChannel;

        if (size <= 0) {
            return;
        }

        for (std::size_t i = 0; i < size; i++) {
            for(std::size_t group = 0; group < _doe.GroupData.size(); group ++) {
                auto offset = group*8;
                _doe.GroupData.at(group).add_at(i, i,
                                         all_chs[offset + 0] == nullptr ? 0 : all_chs[offset + 0][i],
                                         all_chs[offset + 1] == nullptr ? 0 : all_chs[offset + 1][i],
                                         all_chs[offset + 2] == nullptr ? 0 : all_chs[offset + 2][i],
                                         all_chs[offset + 3] == nullptr ? 0 : all_chs[offset + 3][i],
                                         all_chs[offset + 4] == nullptr ? 0 : all_chs[offset + 4][i],
                                         all_chs[offset + 5] == nullptr ? 0 : all_chs[offset + 5][i],
                                         all_chs[offset + 6] == nullptr ? 0 : all_chs[offset + 6][i],
                                         all_chs[offset + 7] == nullptr ? 0 : all_chs[offset + 7][i]);
            }
        }
    }

//    void sipm_voltage_system_update() {
//        static auto get_voltage = make_total_timed_event(
//            std::chrono::seconds(1),
//            [&]() {
//                _sipm_volt_sys.async_get(
//                [=](serial_ptr& port)
//                    -> std::optional<SiPMVoltageMeasure> {
//                    static auto send_msg_slow = make_blocking_total_timed_event(
//                        std::chrono::milliseconds(10),
//                        [&](const std::string& msg){
//                            send_msg(port, msg + "\n", "");
//                        }
//                    );
//
//                    send_msg_slow.ChangeWaitTime(
//                        std::chrono::milliseconds(10));
//                    send_msg_slow("++addr 10");
//                    send_msg_slow(":fetch?");
//                    send_msg_slow("++read eoi");
//
//                    auto volt = retrieve_msg<double>(port);
//
//                    if (!volt) {
//                        return {};
//                    }
//
//                    double time = get_current_time_epoch() / 1000.0;
//                    send_msg_slow.ChangeWaitTime(
//                        std::chrono::milliseconds(100));
//                    send_msg_slow("++addr 22");
//                    send_msg_slow(":fetch?");
//                    send_msg_slow("++read eoi");
//
//                    auto curr = retrieve_msg<double>(port);
//
//                    send_msg_slow(":init");
//
//                    if (!curr) {
//                        return {};
//                    }
//
//                    SiPMVoltageMeasure out;
//                    out.Time = time;
//                    out.Volt = volt.value();
//                    out.Current = curr.value();
//
//                    return std::make_optional(out);
//            });
//
//            auto latest_values = _sipm_volt_sys.async_get_values();
//            for (auto r : latest_values) {
//                double time = r.Time;
//                double volt = r.Volt;
//                double curr = r.Current;
//
//                volt = calculate_sipm_voltage(volt, curr);
//
//                // This measure is a NaN/Overflow from the picoammeter
//                if (curr < 9.9e37){
//                    _doe.LatestMeasure.Volt = volt;
//                    _doe.LatestMeasure.Time = time;
//                    _doe.LatestMeasure.Current = curr;
//                    _voltages_file->Add(_doe.LatestMeasure);
//                    // _indicator_sender(IndicatorNames::DMM_VOLTAGE, time, volt);
//                    // _indicator_sender(IndicatorNames::LATEST_DMM_VOLTAGE, volt);
//                    // _indicator_sender(IndicatorNames::PICO_CURRENT, time, curr);
//                    // _indicator_sender(IndicatorNames::LATEST_PICO_CURRENT, curr);
//                }
//            }
//
//        });
//
//        static auto save_voltages = make_total_timed_event(
//            std::chrono::seconds(30),
//            [&]() {
//                spdlog::info("Saving voltage (Keithley 2000) voltages.");
//                async_save(_voltages_file,
//                [](const SiPMVoltageMeasure& mes) {
//                    std::ostringstream out;
//                    out.precision(7);
//                    out << "," << mes.Volt <<
//                        "," << mes.Current << "\n" << std::scientific;;
//
//                    return  std::to_string(mes.Time) + out.str();
//            });
//        });
//
//        switch (_doe.CurrentState) {
//            case SiPMAcquisitionStates::OscilloscopeMode:
//            case SiPMAcquisitionStates::MeasurementRoutineMode:
//                if (_doe.SiPMVoltageSysChange) {
//                    _sipm_volt_sys.get([&](serial_ptr& port)
//                    -> std::optional<SiPMVoltageMeasure> {
//                        send_msg(port, "++addr 22\n", "");
//
//                        // The wait time to let the user know the voltage
//                        // has stabilized is 90s or 3 mins
//                    #ifdef NDEBUG
//                        _wait_time = 90000.0;
//                    #else
//                        // Debug mode reduces to 30secs. We do not have time!
//                        _wait_time = 30000.0;
//                    #endif
//                        if (_doe.SiPMVoltageSysSupplyEN) {
//                            send_msg(port, ":sour:volt:stat ON\n", "");
//                            spdlog::warn("Turning on voltage supply.");
//
//                        } else {
//                            send_msg(port, ":sour:volt:stat OFF\n", "");
//                            spdlog::warn("Turning off voltage supply.");
//                        }
//
//                        auto dV = std::abs(_doe.LatestMeasure.Volt
//                            - _doe.SiPMVoltageSysVoltage);
//                        if (dV >= 10.0) {
//                            // However, when the voltage swing is higher than
//                            // 10V, it should be multiplied it by 2.5.
//                            _wait_time *= 1.0;
//                        }
//
//                        send_msg(port, ":sour:volt "
//                            + std::to_string(_doe.SiPMVoltageSysVoltage)
//                            + "\n", "");
//
//                        spdlog::info("Changing output voltage to {0}",
//                            _doe.SiPMVoltageSysVoltage);
//
//                        // Resetting the timer.
//                        _reset_timer = get_current_time_epoch(); // ms
//
//                        // Let's also reset this indicator which helps
//                        // minimize confusion in the GUI
//                        // _indicator_sender(IndicatorNames::DONE_DATA_TAKING,
//                        //     false);
//
//                        return {};
//                    });
//
//                    // Reset
//                    _doe.SiPMVoltageSysChange = false;
//                }
//
//                // This is to allow some time between voltage changes so it
//                // settles before a measurement is done.
//                if ((get_current_time_epoch() - _reset_timer) < _wait_time) {
//                    _doe.isVoltageStabilized = false;
//                } else {
//                    _doe.isVoltageStabilized = true;
//                }
//
//                // _indicator_sender(IndicatorNames::CURRENT_STABILIZED,
//                //         _doe.isVoltageStabilized);
//
//                get_voltage();
//                save_voltages();
//            break;
//
//            case SiPMAcquisitionStates::Standby:
//            case SiPMAcquisitionStates::AttemptConnection:
//            case SiPMAcquisitionStates::Disconnected:
//            case SiPMAcquisitionStates::Closing:
//            default:
//            break;
//        }
//    }
};

template<typename Pipes>
auto make_sipmacquisition_manager(const Pipes& p) {
    return std::make_unique<SiPMAcquisitionManager<Pipes>>(p);
}

}  // namespace SBCQueens
#endif
