#ifndef CAENDIGITIZERINTERFACE_H
#define CAENDIGITIZERINTERFACE_H
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
#include "sbcqueens-gui/serial_helper.hpp"
#include "sbcqueens-gui/file_helpers.hpp"
#include "sbcqueens-gui/caen_helper.hpp"
#include "sbcqueens-gui/implot_helpers.hpp"
#include "sbcqueens-gui/indicators.hpp"
#include "sbcqueens-gui/timing_events.hpp"
#include "sbcqueens-gui/armadillo_helpers.hpp"

#include "sbcqueens-gui/hardware_helpers/CAENInterfaceData.hpp"
#include "sbcqueens-gui/hardware_helpers/ClientController.hpp"
#include "sbcqueens-gui/hardware_helpers/Calibration.hpp"

#include "sbcqueens-gui/sipm_helpers/BreakDownRoutine.hpp"
#include "sbcqueens-gui/sipm_helpers/AcquisitionRoutine.hpp"

#include "sipmanalysis/PulseFunctions.hpp"
#include "sipmanalysis/SPEAnalysis.hpp"
#include "sipmanalysis/GainVBDEstimation.hpp"

namespace SBCQueens {

template<typename... Queues>
class CAENDigitizerInterface {
    // Software related items
    std::tuple<Queues&...> _queues;
    CAENInterfaceData _doe;
    IndicatorSender<IndicatorNames> _indicator_sender;
    IndicatorSender<uint8_t> _plot_sender;

    // Files
    std::string _run_name =  "";
    DataFile<SiPMVoltageMeasure> _voltages_file;
    LogFile _saveinfo_file;

    // Hardware
    uint8_t _num_chs;
    double _acq_rate;
    CAEN _caen_port = nullptr;
    ClientController<serial_ptr, SiPMVoltageMeasure> _sipm_volt_sys;

    uint32_t SavedWaveforms = 0;
    uint64_t TriggeredWaveforms = 0;

    bool _vbd_created = false;

    // tmp stuff
    double _wait_time = 900000.0;
    double _reset_timer = 0;
    uint16_t* _data;
    size_t _length;
    std::vector<double> _x_values, _y_values;
    CAENEvent _osc_event;
    // 1024 because the caen can only hold 1024 no matter what model (so far)
    uint32_t LatestAcquiredWaveforms;
    std::array<CAENEvent, 1024> _processing_evts;

    // Analysis
    std::unique_ptr<BreakdownRoutine> _vbd_routine = nullptr;
    GainVBDFitParameters _latest_breakdown_voltage;
    std::unique_ptr<AcquisitionRoutine> _acq_routine = nullptr;

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
    CAENInterfaceState_ptr measurementRoutineMode_state;
    // CAENInterfaceState_ptr runMode_state;
    CAENInterfaceState_ptr disconnected_state;
    CAENInterfaceState_ptr closing_state;

 public:
    explicit CAENDigitizerInterface(Queues&... queues) :
        _queues(forward_as_tuple(queues...)),
        _indicator_sender(std::get<GeneralIndicatorQueue&>(_queues)),
        _plot_sender(std::get<SiPMPlotQueue&>(_queues)),
        _sipm_volt_sys("Keithley 2000/6487") {
        // This is possible because std::function can be assigned
        // to whatever std::bind returns
        standby_state = std::make_shared<CAENInterfaceState>(
            std::chrono::milliseconds(200),
            std::bind(&CAENDigitizerInterface::standby, this));

        attemptConnection_state = std::make_shared<CAENInterfaceState>(
            std::chrono::milliseconds(1),
            std::bind(&CAENDigitizerInterface::attempt_connection, this));

        oscilloscopeMode_state = std::make_shared<CAENInterfaceState>(
            std::chrono::milliseconds(150),
            std::bind(&CAENDigitizerInterface::oscilloscope, this));

        measurementRoutineMode_state = std::make_shared<CAENInterfaceState>(
            std::chrono::milliseconds(50),
            std::bind(&CAENDigitizerInterface::measurement_routine_mode, this));

        // runMode_state = std::make_shared<CAENInterfaceState>(
        //     std::chrono::milliseconds(50),
        //     std::bind(&CAENDigitizerInterface::run_mode, this));

        disconnected_state = std::make_shared<CAENInterfaceState>(
            std::chrono::milliseconds(1),
            std::bind(&CAENDigitizerInterface::disconnected_mode, this));

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

        _sipm_volt_sys.build(
            [=] (serial_ptr& port) -> bool {
                static auto send_msg_slow = make_blocking_total_timed_event(
                    std::chrono::milliseconds(200),
                    [&](const std::string& msg){
                        spdlog::info("Sending msg to Keithely: {0}", msg);
                        send_msg(port, msg + "\n", "");
                    }
                );

                SerialParams ssp;
                ssp.Timeout = serial::Timeout::simpleTimeout(5000);
                ssp.Baudrate = 115200;
                connect_par(port, _doe.SiPMVoltageSysPort, ssp);

                if (!port) {
                    spdlog::error("Port with port name {0} was not open",
                        _doe.SiPMVoltageSysPort);
                    return false;
                }

                if (!port->isOpen()) {
                    spdlog::error("Port with port name {0} was not open",
                        _doe.SiPMVoltageSysPort);
                    return false;
                }

                spdlog::info("Connected to Keithley 2000! "
                    "Initializing...");

                // Setup GPIB interface
                // send_msg_slow("++rst");
                // This should be not run if we are in debug mode.
            #ifdef NDEBUG
                send_msg_slow("++eos 0");
                send_msg_slow("++addr 10");
                send_msg_slow("++auto 0");

                //// if tired of waiting for keithley comment starting here:
                // Now Keithley
                // These parameters are constant for now
                // but later they can become a parameter or the voltage
                // system can be optional at all
                send_msg_slow("*rst");
                send_msg_slow(":init:cont on");
                send_msg_slow(":volt:dc:nplc 10");
                send_msg_slow(":volt:dc:rang:auto 0");
                send_msg_slow(":volt:dc:rang 100");
                send_msg_slow(":volt:dc:aver:stat 0");
                // send_msg_slow(":volt:dc:aver:tcon mov");
                // send_msg_slow(":volt:dc:aver:coun 100");

                send_msg_slow(":form ascii");

                // Keithley 6487
                send_msg_slow("++addr 22");
                send_msg_slow("*rst");
                send_msg_slow(":form:elem read");

                // Volt supply side
                send_msg_slow(":sour:volt:rang 55");
                send_msg_slow(":sour:volt:ilim 250e-6");
                send_msg_slow(":sour:volt "
                    + std::to_string(_doe.SiPMVoltageSysVoltage));
                send_msg_slow(":sour:volt:stat OFF");

                // Ammeter side
                send_msg_slow(":sens:curr:dc:nplc 6");
                send_msg_slow(":sens:aver:stat off");

                // Zero check
                send_msg_slow(":syst:zch ON");
                send_msg_slow(":curr:rang 2E-7");
                send_msg_slow.ChangeWaitTime(std::chrono::milliseconds(1000));
                send_msg_slow(":init");
                send_msg_slow(":syst:zcor:stat OFF");
                send_msg_slow(":syst:zcor:acq");
                send_msg_slow(":syst:zch OFF");
                send_msg_slow(":syst:zcor ON");
                send_msg_slow(":syst:azer ON");

                send_msg_slow.ChangeWaitTime(std::chrono::milliseconds(200));
                send_msg_slow(":arm:coun 1");
                send_msg_slow(":arm:sour imm");
                send_msg_slow(":arm:timer 0");
                send_msg_slow(":trig:coun 1");
                send_msg_slow(":trig:sour imm");

                send_msg_slow.ChangeWaitTime(std::chrono::milliseconds(1100));
                send_msg_slow(":init");
            #endif

                return true;
            },
            [=](serial_ptr& port) -> bool {
                if (port) {
                    send_msg(port, "++addr 22\n", "");
                    send_msg(port, ":sour:volt:stat OFF\n", "");
                    send_msg(port, ":sour:volt 0.0\n", "");

                    if (port->isOpen()) {
                        disconnect(port);
                    }
                }

                return false;
        });

        // Actual loop!
        while (main_loop_state->operator()()) {
            sipm_voltage_system_update();
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
        _indicator_sender(IndicatorNames::TRIGGERRATE, dWaveforms / dt);
    }

    // Local error checking. Checks if there was an error and prints it
    // using spdlog
    bool lec() {
        return check_error(_caen_port, [](const std::string& cmd) {
            spdlog::error(cmd);
        });
    }

    void switch_state(const CAENInterfaceStates& newState) {
        _doe.CurrentState = newState;
        switch (_doe.CurrentState) {
            case CAENInterfaceStates::Standby:
                main_loop_state = standby_state;
            break;

            case CAENInterfaceStates::AttemptConnection:
                main_loop_state = attemptConnection_state;
            break;

            case CAENInterfaceStates::OscilloscopeMode:
                main_loop_state = oscilloscopeMode_state;
            break;

            case CAENInterfaceStates::MeasurementRoutineMode:
                main_loop_state = measurementRoutineMode_state;
            break;

            // case CAENInterfaceStates::RunMode:
            //     main_loop_state = runMode_state;
            // break;

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
            if (!task(_doe)) {
                spdlog::warn("Something went wrong with a command!");
            } else {
                switch_state(_doe.CurrentState);
                return true;
            }
        }

        return false;
    }

    // Logic to disconnect the CAEN and clear up memory
    void close_caen() {
        auto err = disconnect(_caen_port);
        check_error(err, [](const std::string& cmd) {
            spdlog::error(cmd);
        });
    }

    // Does nothing other than wait 100ms to avoid clogging PC resources.
    bool standby() {
        change_state();
        return true;
    }

    // Attempts a connection to the CAEN digitizer, setups the channels,
    // starts acquisition, and moves to the oscilloscope mode
    bool attempt_connection() {
        // If the port is open, clean up and close the port.
        // This will help turn attempt_connection into a reset
        // for the entire system.
        if (_caen_port) {
            close_caen();
        }

        auto err = connect(_caen_port,
            _doe.Model,
            _doe.ConnectionType,
            _doe.PortNum,
            0,
            _doe.VMEAddress);

        // If port resource was not created, it equals a failure!
        if (!_caen_port) {
            // Print what was the error
            check_error(err, [](const std::string& cmd) {
                spdlog::error(cmd);
            });

            // We disconnect because this will free resources (in case
            // they were created)
            disconnect(_caen_port);
            switch_state(CAENInterfaceStates::Standby);
            return true;
        }

        spdlog::info("Connected to CAEN Digitizer!");

        setup(_caen_port, _doe.GlobalConfig, _doe.GroupConfigs);

        _num_chs = _caen_port->ModelConstants.NumChannels;
        _acq_rate = _caen_port->ModelConstants.AcquisitionRate;

        // Enable acquisition HAS to be called AFTER setup
        enable_acquisition(_caen_port);

        // Trying to call once will initialize the port
        // but really this call is not important
        // _sipm_volt_sys.get([=](serial_ptr& port) -> std::optional<double> {
        //     return {};
        // });
        std::generate(_processing_evts.begin(), _processing_evts.end(),
        [&](){
            return std::make_shared<caenEvent>(_caen_port->Handle);
        });

        // Allocate memory for events
        _osc_event = std::make_shared<caenEvent>(_caen_port->Handle);

        auto failed = lec();

        if (failed) {
            spdlog::warn("Failed to setup CAEN");
            err = disconnect(_caen_port);
            check_error(err, [](const std::string& cmd) {
                spdlog::error(cmd);
            });

            switch_state(CAENInterfaceStates::Standby);
        } else {
            spdlog::info("CAEN Setup complete!");
            switch_state(CAENInterfaceStates::OscilloscopeMode);
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

        open(_saveinfo_file,
            _doe.RunDir + "/" + _run_name + "/SaveInfo.txt");
        bool s = (_saveinfo_file != nullptr);

        if (!s) {
            spdlog::error("Failed to open SaveInfo.txt");
            switch_state(CAENInterfaceStates::Disconnected);
        }

        spdlog::info("Trying to open file {0}", _doe.RunDir
            + "/" + _run_name
            + "/SiPMIV.txt");
        open(_voltages_file, _doe.RunDir
            + "/" + _run_name
            + "/SiPMIV.txt");

        s = _voltages_file > 0;
        if (!s) {
            spdlog::error("Failed to open voltage file");
        }

        std::filesystem::create_directory(_doe.RunDir
            + "/" + _run_name);

        change_state();
        return true;
    }

    // While in this state it shares the data with the GUI but
    // no actual file saving is happening. It essentially serves
    // as a mode in where the user can see what is happening.
    // Similar to an oscilloscope
    bool oscilloscope() {
        auto events = get_events_in_buffer(_caen_port);
        _indicator_sender(IndicatorNames::CAENBUFFEREVENTS, events);

        retrieve_data(_caen_port);

        if (_doe.SoftwareTrigger) {
            software_trigger(_caen_port);
            _doe.SoftwareTrigger = false;
        }

        if (_caen_port->Data.DataSize > 0 && _caen_port->Data.NumEvents > 0) {
            // TriggeredWaveforms += _caen_port->Data.NumEvents;
            // Due to the slow rate oscilloscope mode is happening, using
            // events instead of ..->Data.NumEvents is a better number to use
            // to approximate the actual number of waveforms acquired.
            // Specially because the buffer is cleared.
            TriggeredWaveforms += events;
            // spdlog::info("Total size buffer: {0}",  Port->Data.TotalSizeBuffer);
            // spdlog::info("Data size: {0}", Port->Data.DataSize);
            // spdlog::info("Num events: {0}", Port->Data.NumEvents);

            extract_event(_caen_port, 0, _osc_event);

            // spdlog::info("Event size: {0}", osc_event->Info.EventSize);
            // spdlog::info("Event counter: {0}", osc_event->Info.EventCounter);
            // spdlog::info("Trigger Time Tag: {0}",
            //     osc_event->Info.TriggerTimeTag);

            process_data_for_gui();

            // Clear events in buffer
            clear_data(_caen_port);
        }

        lec();
        change_state();
        return true;
    }

    // Does the SiPM pulse processing but no file saving
    // is done. Serves more for the user to know
    // how things are changing.
    bool measurement_routine_mode() {
        if (not _vbd_routine) {
            _vbd_routine = std::make_unique<BreakdownRoutine>(
                _caen_port, _doe, _run_name, _saveinfo_file);

            // Reset all indicators for this section
            _indicator_sender(IndicatorNames::FINISHED_ROUTINE, false);
            _indicator_sender(IndicatorNames::MEASUREMENT_ROUTINE_ONGOING,
                false);
            _indicator_sender(IndicatorNames::BREAKDOWN_ROUTINE_ONGOING,
                false);
        }

        _vbd_routine->update();

        if(_vbd_routine->hasVoltageChanged()) {
            _doe.SiPMVoltageSysVoltage = _vbd_routine->getCurrentVoltage();
            _doe.SiPMVoltageSysChange = true;
        }

        _processing_evts = _vbd_routine->getLatestEvents();
        rdm_extract_for_gui();

        // GUI related stuff
        LatestAcquiredWaveforms = _vbd_routine->getLatestNumEvents();
        TriggeredWaveforms += static_cast<uint64_t>(LatestAcquiredWaveforms);
        SavedWaveforms = _vbd_routine->getTotalAcquiredEvents();
        _indicator_sender(IndicatorNames::SAVED_WAVEFORMS, SavedWaveforms);

        switch (_vbd_routine->getCurrentState()) {
        case BreakdownRoutineState::Finished:
            switch_state(CAENInterfaceStates::OscilloscopeMode);

            _indicator_sender(IndicatorNames::FINISHED_ROUTINE, true);

            // Set back to 52 just in case.
            _doe.SiPMVoltageSysVoltage = *_vbd_routine->GainVoltages.cbegin();
            _doe.SiPMVoltageSysChange = true;
            // Clean resources and get the port back
            // _caen_port = _vbd_routine->retrieveCAEN();
            _vbd_routine = nullptr;
            break;
        case BreakdownRoutineState::GainMeasurements:
            _indicator_sender(IndicatorNames::BREAKDOWN_ROUTINE_ONGOING, true);
        case BreakdownRoutineState::CalculateBreakdownVoltage:
            if (_vbd_routine->hasNewGainMeasurement())
            {
                auto pars = _vbd_routine->getAnalysisLatestValues();
                _indicator_sender(IndicatorNames::GAIN_VS_VOLTAGE,
                    _doe.LatestMeasure.Volt, pars.SPEParameters(1));
                _indicator_sender(IndicatorNames::SPE_GAIN_MEAN,
                pars.SPEParameters(1));

                _indicator_sender(IndicatorNames::SPE_EFFICIENCTY,
                    pars.SPEEfficiency);
                _indicator_sender(IndicatorNames::INTEGRAL_THRESHOLD,
                    pars.IntegralThreshold);

                _indicator_sender(IndicatorNames::OFFSET,
                    pars.SPEParameters(2));

                _indicator_sender(IndicatorNames::RISE_TIME,
                    pars.SPEParameters(3));

                _indicator_sender(IndicatorNames::FALL_TIME,
                    pars.SPEParameters(4));
            }
            break;
        case BreakdownRoutineState::Acquisition:
            _indicator_sender(IndicatorNames::MEASUREMENT_ROUTINE_ONGOING, true);
            if (_vbd_routine->hasNewBreakdownVoltage()) {
               auto values = _vbd_routine->getBreakdownVoltage();

                _indicator_sender(IndicatorNames::BREAKDOWN_VOLTAGE,
                    values.BreakdownVoltage);
                _indicator_sender(IndicatorNames::BREAKDOWN_VOLTAGE_ERR,
                    values.BreakdownVoltageError);
            }

            break;
        default:
        break;
        }

        // Cancel button routine
        if (_doe.CancelMeasurements && _vbd_routine) {
            switch_state(CAENInterfaceStates::OscilloscopeMode);

            // _caen_port = _vbd_routine->retrieveCAEN();
            _vbd_routine = nullptr;
            _doe.CancelMeasurements = false;
        }

        change_state();
        return true;
    }

    // bool run_mode() {
    //     if (not _acq_routine) {
    //         _indicator_sender(IndicatorNames::DONE_DATA_TAKING, false);
    //         _acq_routine = std::make_unique<AcquisitionRoutine> (
    //             _caen_port, _doe, _run_name, _saveinfo_file,
    //             _latest_breakdown_voltage);
    //     }

    //     _acq_routine->update();

    //     if(_acq_routine->hasVoltageChanged()) {
    //         _doe.SiPMVoltageSysVoltage = _acq_routine->getCurrentVoltage();
    //         _doe.SiPMVoltageSysChange = true;
    //     }

    //     _processing_evts = _acq_routine->getLatestEvents();
    //     rdm_extract_for_gui();

    //     // GUI related stuff
    //     TriggeredWaveforms = _acq_routine->getLatestNumEvents();
    //     SavedWaveforms = _acq_routine->getTotalAcquiredEvents();
    //     _indicator_sender(IndicatorNames::SAVED_WAVEFORMS, SavedWaveforms);

    //     // It finished, we move back to OscilloscopeMode
    //     if (_acq_routine->hasFinished()) {
    //         switch_state(CAENInterfaceStates::OscilloscopeMode);

    //         // _caen_port = _acq_routine->retrieveCAEN();
    //         _acq_routine = nullptr;

    //         _indicator_sender(IndicatorNames::DONE_DATA_TAKING, true);
    //     }

    //     // Cancel button routine
    //     if (_doe.CancelMeasurements && _acq_routine) {
    //         switch_state(CAENInterfaceStates::OscilloscopeMode);

    //         // _caen_port = _acq_routine->retrieveCAEN();
    //         _acq_routine = nullptr;
    //         _doe.CancelMeasurements = false;
    //     }

    //     change_state();
    //     return true;
    // }

    bool disconnected_mode() {
        spdlog::warn("Manually losing connection to the CAEN digitizer.");
        close_caen();
        switch_state(CAENInterfaceStates::Standby);
        return true;
    }

    // Only mode that stops the main_loop and frees all resources
    bool closing_mode() {
        spdlog::info("Going to close the CAEN thread.");
        close_caen();
        return false;
    }

    void rdm_extract_for_gui() {
        static auto rdm_extract_timed = make_total_timed_event(
            std::chrono::milliseconds(200),
            [&]() {
                // For the GUI
                if (LatestAcquiredWaveforms == 0) {
                    return;
                }
                static std::default_random_engine generator;
                std::uniform_int_distribution<uint64_t>
                    distribution(0, LatestAcquiredWaveforms);
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

        for (std::size_t j = 0; j < _num_chs; j++) {
            // auto& ch_num = ch.second.Number;
            auto buf = _osc_event->Data->DataChannel[j];
            auto size = _osc_event->Data->ChSize[j];

            if (size <= 0) {
                continue;
            }

            _x_values.resize(size);
            _y_values.resize(size);

            for (uint32_t i = 0; i < size; i++) {
                _x_values[i] = i*(1e9/_acq_rate);
                _y_values[i] = static_cast<double>(buf[i]);
            }

            _plot_sender(static_cast<uint8_t>(j),
                _x_values,
                _y_values);
        }
    }

    void sipm_voltage_system_update() {
        static auto get_voltage = make_total_timed_event(
            std::chrono::seconds(1),
            [&]() {
                _sipm_volt_sys.async_get(
                [=](serial_ptr& port)
                    -> std::optional<SiPMVoltageMeasure> {
                    static auto send_msg_slow = make_blocking_total_timed_event(
                        std::chrono::milliseconds(10),
                        [&](const std::string& msg){
                            send_msg(port, msg + "\n", "");
                        }
                    );

                    send_msg_slow.ChangeWaitTime(
                        std::chrono::milliseconds(10));
                    send_msg_slow("++addr 10");
                    send_msg_slow(":fetch?");
                    send_msg_slow("++read eoi");

                    auto volt = retrieve_msg<double>(port);

                    if (!volt) {
                        return {};
                    }

                    double time = get_current_time_epoch() / 1000.0;
                    send_msg_slow.ChangeWaitTime(
                        std::chrono::milliseconds(100));
                    send_msg_slow("++addr 22");
                    send_msg_slow(":fetch?");
                    send_msg_slow("++read eoi");

                    auto curr = retrieve_msg<double>(port);

                    send_msg_slow(":init");

                    if (!curr) {
                        return {};
                    }

                    SiPMVoltageMeasure out;
                    out.Time = time;
                    out.Volt = volt.value();
                    out.Current = curr.value();

                    return std::make_optional(out);
            });

            auto latest_values = _sipm_volt_sys.async_get_values();
            for (auto r : latest_values) {
                double time = r.Time;
                double volt = r.Volt;
                double curr = r.Current;

                volt = calculate_sipm_voltage(volt, curr);

                // This measure is a NaN/Overflow from the picoammeter
                if (curr < 9.9e37){
                    _doe.LatestMeasure.Volt = volt;
                    _doe.LatestMeasure.Time = time;
                    _doe.LatestMeasure.Current = curr;
                    _voltages_file->Add(_doe.LatestMeasure);
                    _indicator_sender(IndicatorNames::DMM_VOLTAGE, time, volt);
                    _indicator_sender(IndicatorNames::LATEST_DMM_VOLTAGE, volt);
                    _indicator_sender(IndicatorNames::PICO_CURRENT, time, curr);
                    _indicator_sender(IndicatorNames::LATEST_PICO_CURRENT, curr);
                }
            }

        });

        static auto save_voltages = make_total_timed_event(
            std::chrono::seconds(30),
            [&]() {
                spdlog::info("Saving voltage (Keithley 2000) voltages.");
                async_save(_voltages_file,
                [](const SiPMVoltageMeasure& mes) {
                    std::ostringstream out;
                    out.precision(7);
                    out << "," << mes.Volt <<
                        "," << mes.Current << "\n" << std::scientific;;

                    return  std::to_string(mes.Time) + out.str();
            });
        });

        switch (_doe.CurrentState) {
            case CAENInterfaceStates::OscilloscopeMode:
            case CAENInterfaceStates::MeasurementRoutineMode:
                if (_doe.SiPMVoltageSysChange) {
                    _sipm_volt_sys.get([&](serial_ptr& port)
                    -> std::optional<SiPMVoltageMeasure> {
                        send_msg(port, "++addr 22\n", "");

                        // The wait time to let the user know the voltage
                        // has stabilized is 90s or 3 mins
                    #ifdef NDEBUG
                        _wait_time = 90000.0;
                    #else
                        // Debug mode reduces to 30secs. We do not have time!
                        _wait_time = 30000.0;
                    #endif
                        if (_doe.SiPMVoltageSysSupplyEN) {
                            send_msg(port, ":sour:volt:stat ON\n", "");
                            spdlog::warn("Turning on voltage supply.");

                        } else {
                            send_msg(port, ":sour:volt:stat OFF\n", "");
                            spdlog::warn("Turning off voltage supply.");
                        }

                        auto dV = std::abs(_doe.LatestMeasure.Volt
                            - _doe.SiPMVoltageSysVoltage);
                        if (dV >= 10.0) {
                            // However, when the voltage swing is higher than
                            // 10V, it should be multiplied it by 2.5.
                            _wait_time *= 2.0;
                        }

                        send_msg(port, ":sour:volt "
                            + std::to_string(_doe.SiPMVoltageSysVoltage)
                            + "\n", "");

                        spdlog::info("Changing output voltage to {0}",
                            _doe.SiPMVoltageSysVoltage);

                        // Resetting the timer.
                        _reset_timer = get_current_time_epoch(); // ms

                        // Let's also reset this indicator which helps
                        // minimize confusion in the GUI
                        _indicator_sender(IndicatorNames::DONE_DATA_TAKING,
                            false);

                        return {};
                    });

                    // Reset
                    _doe.SiPMVoltageSysChange = false;
                }

                // This is to allow some time between voltage changes so it
                // settles before a measurement is done.
                if ((get_current_time_epoch() - _reset_timer) < _wait_time) {
                    _doe.isVoltageStabilized = false;
                } else {
                    _doe.isVoltageStabilized = true;
                }

                _indicator_sender(IndicatorNames::CURRENT_STABILIZED,
                        _doe.isVoltageStabilized);

                get_voltage();
                save_voltages();
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
};

}  // namespace SBCQueens
#endif
