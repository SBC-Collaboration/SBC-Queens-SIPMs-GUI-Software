#ifndef CAENDIGITIZERINTERFACE_H
#define CAENDIGITIZERINTERFACE_H
#include <future>
#include <optional>
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

#include "sbcqueens-gui/hardware_helpers/ClientController.hpp"
#include "sbcqueens-gui/hardware_helpers/Calibration.hpp"

#include "sipmanalysis/PulseFunctions.hpp"
#include "sipmanalysis/SPEAnalysis.hpp"
#include "sipmanalysis/GainVBDEstimation.hpp"

namespace SBCQueens {

struct SaveFileInfo {
    double Time;
    std::string FileName;

    SaveFileInfo() {}
    SaveFileInfo(const double& time, const std::string& fn) :
        Time(time), FileName(fn) {}
};

struct SiPMVoltageMeasure {
    double Current;
    double Volt;
    double Time;  // in unix timestamp
};

enum class CAENInterfaceStates {
    NullState = 0,
    Standby,
    AttemptConnection,
    OscilloscopeMode,
    BreakdownVoltageMode,
    RunMode,
    Disconnected,
    Closing
};

enum class VBRState {
    Idle = 0,
    Init,
    Analysis,
    CalculateBreakdownVoltage,
    Reset,
    ResetAll
};

struct BreakdownVoltageConfigData {
    uint32_t SPEEstimationTotalPulses = 10000;
    uint32_t DataPulses = 1000000;

    VBRState State = VBRState::Idle;
};

struct CAENInterfaceData {
    std::string RunDir = "";

    CAENConnectionType ConnectionType;

    CAENDigitizerModel Model;
    CAENGlobalConfig GlobalConfig;
    std::vector<CAENGroupConfig> GroupConfigs;

    int PortNum = 0;
    uint32_t VMEAddress = 0;

    CAENInterfaceStates CurrentState = CAENInterfaceStates::NullState;

    bool SoftwareTrigger = false;
    bool ResetCaen = false;

    std::string SiPMVoltageSysPort = "";
    int SiPMID = 0;
    int CellNumber = 0;
    float LatestTemperature = -1.f;
    bool SiPMVoltageSysChange = false;
    bool SiPMVoltageSysSupplyEN = false;
    float SiPMVoltageSysVoltage = 0.0;

    std::string SiPMName = "";
    BreakdownVoltageConfigData VBDData;
};

using CAENQueueType = std::function < bool(CAENInterfaceData&) >;
using CAENQueue = moodycamel::ReaderWriterQueue< CAENQueueType >;

template<typename... Queues>
class CAENDigitizerInterface {
    // Software related items
    std::tuple<Queues&...> _queues;
    CAENInterfaceData _doe;
    IndicatorSender<IndicatorNames> _indicator_sender;
    IndicatorSender<uint8_t> _plot_sender;

    // Files
    std::string _run_name =  "";
    DataFile<CAENEvent> _pulse_file;
    DataFile<SiPMVoltageMeasure> _voltages_file;
    DataFile<SaveFileInfo> _saveinfo_file;

    // Hardware
    CAEN _caen_port = nullptr;
    ClientController<serial_ptr, SiPMVoltageMeasure> _sipm_volt_sys;

    SiPMVoltageMeasure _latest_measure;
    uint64_t SavedWaveforms = 0;
    uint64_t TriggeredWaveforms = 0;

    // tmp stuff
    double _reset_timer = 20000;
    uint16_t* _data;
    size_t _length;
    std::vector<double> _x_values, _y_values;
    CAENEvent _osc_event, _adj_osc_event;
    // 1024 because the caen can only hold 1024 no matter what model (so far)
    CAENEvent _processing_evts[1024];

    // Analysis
    arma::mat _coords;
    std::unique_ptr< SPEAnalysis<SimplifiedSiPMFunction> > _spe_analysis;
    std::unique_ptr< GainVBDEstimation > _vbe_analysis;

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
    CAENInterfaceState_ptr breakdownVoltageMode_state;
    CAENInterfaceState_ptr runMode_state;
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

        breakdownVoltageMode_state = std::make_shared<CAENInterfaceState>(
            std::chrono::milliseconds(1),
            std::bind(&CAENDigitizerInterface::breakdown_voltage_mode, this));

        runMode_state = std::make_shared<CAENInterfaceState>(
            std::chrono::milliseconds(1),
            std::bind(&CAENDigitizerInterface::run_mode, this));

        disconnected_state = std::make_shared<CAENInterfaceState>(
            std::chrono::milliseconds(1),
            std::bind(&CAENDigitizerInterface::disconnected_mode, this));

        closing_state = std::make_shared<CAENInterfaceState>(
            std::chrono::milliseconds(1),
            std::bind(&CAENDigitizerInterface::closing_mode, this));

        _coords = arma::mat(5, 1, arma::fill::ones);
    }

    // No copying
    CAENDigitizerInterface(const CAENDigitizerInterface&) = delete;

    ~CAENDigitizerInterface() {}

    void operator()() {
        spdlog::info("Initializing CAEN thread");

        main_loop_state = standby_state;
        _vbe_analysis.reset(new GainVBDEstimation());

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
                // send_msg_slow(port, "++rst");
                send_msg_slow("++eos 0");
                send_msg_slow("++addr 10");
                send_msg_slow("++auto 0");

                //// if tired of waiting for keithley comment starting here:
                // Now Keithley
                // These parameters are constant for now
                // but later they can become a parameter or the voltage
                // system can be optional at all
                // send_msg_slow("*rst");
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
                // send_msg_slow("*rst");
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

            case CAENInterfaceStates::BreakdownVoltageMode:
                main_loop_state = breakdownVoltageMode_state;
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
        for (CAENEvent& evt : _processing_evts) {
            if (evt) {
                evt.reset();
            }
        }

        if (_osc_event) {
            _osc_event.reset();
        }

        if (_adj_osc_event) {
            _adj_osc_event.reset();
        }

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

        // Enable acquisition HAS to be called AFTER setup
        enable_acquisition(_caen_port);

        // Trying to call once will initialize the port
        // but really this call is not important
        // _sipm_volt_sys.get([=](serial_ptr& port) -> std::optional<double> {
        //     return {};
        // });

        // Allocate memory for events
        _osc_event = std::make_shared<caenEvent>(_caen_port->Handle);

        for (CAENEvent& evt : _processing_evts) {
            evt = std::make_shared<caenEvent>(_caen_port->Handle);
        }

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

        open(_saveinfo_file,
            _doe.RunDir + "/" + _run_name + "/SaveInfo.txt");
        bool s = (_saveinfo_file != nullptr);

        if (!s) {
            spdlog::error("Failed to open SaveInfo.txt");
            switch_state(CAENInterfaceStates::Disconnected);
        }

        spdlog::info("Trying to open file {0}", _doe.RunDir
            + "/" + _run_name
            + "/SIPM_VOLTAGES.txt");
        open(_voltages_file, _doe.RunDir
            + "/" + _run_name
            + "/SIPM_VOLTAGES.txt");

        s = _voltages_file > 0;
        if (!s) {
            spdlog::error("Failed to open voltage file");
        }

        std::ostringstream out;
        auto today = date::year_month_day{
            date::floor<date::days>(std::chrono::system_clock::now())};
        out << today;
        _run_name = out.str();

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
    bool breakdown_voltage_mode() {
        static std::string file_name = "";
        static uint32_t num_events = 0;
        static auto process_events = [&]() -> bool {
            bool isData = retrieve_data_until_n_events(_caen_port,
                _caen_port->GlobalConfig.MaxEventsPerRead);

            // While all of this is happening, the digitizer is taking data
            if (!isData) {
                return false;
            }

            // double frequency = 0.0;
            num_events = _caen_port->Data.NumEvents;
            TriggeredWaveforms += num_events;
            for (uint32_t i = 0; i < num_events; i++) {
                // Extract event i
                extract_event(_caen_port, i, _processing_evts[i]);
            }

            return true;
        };

        bool new_data = process_events();

        // startup!
        static arma::mat spe_estimation_pulse_buffer;
        static uint32_t acq_pulses = 0;
        static bool init_done = false;
        // This is to allow some time between voltage changes so it settles
        // before a measurement is done.

        if (_reset_timer > 0) {
            _reset_timer -= get_current_time_epoch();
        }

        switch (_doe.VBDData.State) {
            case VBRState::Init:
            {
                if (!init_done && _reset_timer <= 0.0) {
                    // Memory allocations and prepare the resources for whenever
                    // the user presses the next button, and prepares the file

                    // File preparation
                    std::ostringstream out;
                    out.precision(3);
                    out << _doe.LatestTemperature << "degC_";
                    out << _doe.SiPMVoltageSysVoltage << "V";
                    file_name = _doe.RunDir
                        + "/" + _run_name
                        + "/" + std::to_string(_doe.SiPMID) + "_"
                        + std::to_string(_doe.CellNumber) + "cell_"
                        + out.str()
                        + "_spe_estimation.bin";

                    open(_pulse_file,
                        file_name,
                        // sbc_init_file is a function that saves the header
                        // of the sbc data format as a function of record length
                        // and number of channels
                        sbc_init_file,
                        _caen_port);

                    // Save into the log when the data saving started
                    double t = get_current_time_epoch() / 1000.0;
                    _saveinfo_file->Add(SaveFileInfo(t, file_name));

                    // It will hold this many pulses so let's allocate what we need
                    spe_estimation_pulse_buffer.resize(
                        _doe.VBDData.SPEEstimationTotalPulses,
                        _doe.GlobalConfig.RecordLength);

                    // To get the prepulse region in sp, we fuse post trigger %
                    // turn it into pre-trigger %
                    double prepulse_end_region =
                        1.0 - 0.01*_doe.GlobalConfig.PostTriggerPorcentage;

                    // Multiply by RecordLength to turn into sp
                    // and subtract by the trigger lag roughly 125. Not accurate
                    prepulse_end_region
                        = _doe.GlobalConfig.RecordLength*prepulse_end_region
                        - 125.0;  // the 70 is a constant delay in the trigger

                    // We limit it to 0 and turn it into a uint32_t
                    uint32_t prepulse_end_region_u = prepulse_end_region < 0 ?
                        0 : static_cast<uint32_t>(prepulse_end_region);

                    // We log the expected t0
                    t = get_current_time_epoch() / 1000.0;
                    _saveinfo_file->Add(SaveFileInfo(t,
                        "expected_t0 : "
                        + std::to_string(prepulse_end_region_u + 1)));

                    // _coords is the guesses for the analysis
                    // 0 being the t0 guess
                    _coords(0, 0) = prepulse_end_region + 1;
                    // 1 is the gain guess
                    _coords(1, 0) = 35e3;
                    // 2 is the baseline guess
                    _coords(2, 0) = v_threshold_cts_to_adc_cts(_caen_port,
                        _doe.GroupConfigs[0].DCOffset);
                    // fall time
                    _coords(3, 0) = 20;
                    // rise time
                    _coords(4, 0) = 5;

                    spdlog::info("Expected analysis t0: {0}",
                        prepulse_end_region_u);

                    // We limit the analysis to a window of 400sp to speed up
                    // the calculations
                    arma::uword window =
                        _doe.GlobalConfig.RecordLength - prepulse_end_region_u;

                    window = std::clamp(window, window, 400ull);

                    // We also log the tf to keep a record of it
                    _saveinfo_file->Add(SaveFileInfo(t,
                        "window : "
                        + std::to_string(window)));

                    spdlog::info("Analysis window: {0}",
                        window);

                    // We create the analysis object with all the info, but
                    // this does not do the analysis!
                    _spe_analysis.reset(new SPEAnalysis<SimplifiedSiPMFunction>(
                        prepulse_end_region_u,
                        window, _coords));

                    acq_pulses = 0;
                    init_done = true;
                    SavedWaveforms = 0;

                    // Front end preparation
                    _indicator_sender(IndicatorNames::FULL_ANALYSIS_DONE, false);
                    _indicator_sender(IndicatorNames::CALCULATING_VBD, false);
                    _indicator_sender(IndicatorNames::ANALYSIS_ONGOING, true);

                    _doe.VBDData.State = VBRState::Analysis;

                    async_save(_saveinfo_file, [](const SaveFileInfo& val){
                        return std::to_string(val.Time) + "," + val.FileName
                            + "\n";
                    });
                } else {
                    _doe.VBDData.State = VBRState::Idle;
                }
            }
            break;
            case VBRState::Analysis:
                // This case can stall if no data is coming in, maybe
                // add a timeout later.
                if (new_data) {
                    // k = 1, we always throw the first event
                    for (uint32_t k = 1; k < num_events; k++) {

                        // If we already have enough events, do not grab more
                        if (acq_pulses >= _doe.VBDData.SPEEstimationTotalPulses ) {
                            continue;
                        }

                        // If the next even time is smaller than the previous
                        // it means the trigger tag overflowed. We ignore it
                        // because I do not feel like making the math right now
                        if (_processing_evts[k]->Info.TriggerTimeTag <
                            _processing_evts[k - 1]->Info.TriggerTimeTag) {
                            continue;
                        }

                        auto dtsp = _processing_evts[k]->Info.TriggerTimeTag
                            - _processing_evts[k - 1]->Info.TriggerTimeTag;

                        // 1 tsp = 8 ns as per CAEN documentation
                        // dtsp * 8ns = time difference in ns
                        // In other words, do not add events with a time
                        // difference between the last pulse of 1000ns
                        if (dtsp*8 < 1000) {
                            continue;
                        }

                        if (_pulse_file) {
                            _pulse_file->Add(_processing_evts[k]);
                        }

                        arma::mat wave = caen_event_to_armadillo(
                            _processing_evts[k], 1);
                        spe_estimation_pulse_buffer.row(acq_pulses)
                            = wave;

                        acq_pulses++;
                    }

                    // This is to update the indicator
                    SavedWaveforms = acq_pulses;
                    save(_pulse_file, sbc_save_func, _caen_port);
                    if (acq_pulses >= _doe.VBDData.SPEEstimationTotalPulses) {

                        // This one is to note when the data taking ended
                        double t = get_current_time_epoch() / 1000.0;
                        _saveinfo_file->Add(SaveFileInfo(t, file_name));

                        spdlog::info("Finished taking data! Moving to analysis.");
                        // This is the line of code that runs the analysis!
                        auto pars = _spe_analysis->FullAnalysis(
                            spe_estimation_pulse_buffer,
                            _doe.GroupConfigs[0].TriggerThreshold, 1.0);

                        spdlog::info("Finished analysis.");

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

                        // Let the front-end that the analysis is done
                        _indicator_sender(IndicatorNames::FULL_ANALYSIS_DONE,
                            true);
                        _indicator_sender(IndicatorNames::ANALYSIS_ONGOING,
                            false);

                        // Sometimes the analysis can fail and this is
                        // reflected in SPEEfficiency being 0
                        if (pars.SPEEfficiency != 0.0) {
                            _indicator_sender(IndicatorNames::GAIN_VS_VOLTAGE,
                                _latest_measure.Volt,
                                pars.SPEParameters(1));

                            _vbe_analysis->add(pars.SPEParameters(1),
                                pars.SPEParametersErrors(1),
                                _latest_measure.Volt);

                            t = get_current_time_epoch() / 1000.0;
                            for (arma::uword i = 0;
                                i < pars.SPEParameters.n_elem; i++) {
                                _saveinfo_file->Add(
                                    SaveFileInfo(t,
                                std::to_string(pars.SPEParameters(i))));

                                _saveinfo_file->Add(
                                    SaveFileInfo(t,
                                std::to_string(pars.SPEParametersErrors(i))));
                            }
                        }

                        async_save(_saveinfo_file, [](const SaveFileInfo& val) {
                            return std::to_string(val.Time) + ","
                                + val.FileName + "\n";
                        });

                        _doe.VBDData.State = VBRState::Idle;
                    }
                }

            break;
            case VBRState::CalculateBreakdownVoltage:
            {
                if (_vbe_analysis->size() >= 3) {
                    double t = get_current_time_epoch() / 1000.0;
                    auto values = _vbe_analysis->calculate();

                    if (values.BreakdownVoltageError != 0.0) {
                        _indicator_sender(IndicatorNames::BREAKDOWN_VOLTAGE,
                            values.BreakdownVoltage);
                        _indicator_sender(IndicatorNames::BREAKDOWN_VOLTAGE_ERR,
                            values.BreakdownVoltageError);
                        _indicator_sender(IndicatorNames::CALCULATING_VBD, true);

                        _saveinfo_file->Add(
                                    SaveFileInfo(t, "breakdown_voltage : " +
                                std::to_string(values.BreakdownVoltage)));

                        _saveinfo_file->Add(
                                    SaveFileInfo(t, "breakdown_voltage_std : " +
                                std::to_string(values.BreakdownVoltageError)));

                        _saveinfo_file->Add(
                                    SaveFileInfo(t, "dgain_dV : " +
                                std::to_string(values.Rate)));

                        _saveinfo_file->Add(
                                    SaveFileInfo(t, "dgain_dV_std : " +
                                std::to_string(values.RateError)));
                    }
                }

                _doe.VBDData.State = VBRState::Idle;
            }
            break;
            case VBRState::ResetAll:
                _indicator_sender(IndicatorNames::CALCULATING_VBD, false);
                _vbe_analysis.reset(new GainVBDEstimation());
            case VBRState::Reset:
                close(_pulse_file);
                init_done = false;
                SavedWaveforms = 0;
                acq_pulses = 0;
                _doe.VBDData.State = VBRState::Idle;
                _indicator_sender(IndicatorNames::FULL_ANALYSIS_DONE, false);
            break;
            case VBRState::Idle:
            default:
            break;
        }

        static auto extract_for_gui_nb = make_total_timed_event(
            std::chrono::milliseconds(200),
            std::bind(&CAENDigitizerInterface::rdm_extract_for_gui, this));

        static auto checkerror = make_total_timed_event(
            std::chrono::seconds(1),
            std::bind(&CAENDigitizerInterface::lec, this));

        _indicator_sender(IndicatorNames::SAVED_WAVEFORMS,
                    SavedWaveforms);
        checkerror();
        if (new_data) {
            extract_for_gui_nb();
        }

        change_state();
        return true;
    }

    bool run_mode() {
        static bool isFileOpen = false;
        static std::string file_name;
        static auto extract_for_gui_nb = make_total_timed_event(
            std::chrono::milliseconds(200),
            std::bind(&CAENDigitizerInterface::rdm_extract_for_gui, this));

        static auto process_events = [&]() {
            bool isData = retrieve_data_until_n_events(_caen_port,
                _caen_port->GlobalConfig.MaxEventsPerRead);

            // While all of this is happening, the digitizer is taking data
            if (!isData) {
                return;
            }

            for (uint32_t k = 0; k < _caen_port->Data.NumEvents; k++) {
                // Extract event i
                extract_event(_caen_port, k, _processing_evts[k]);
            }

            for (uint32_t k = 1; k < _caen_port->Data.NumEvents; k++) {
                          // If we already have enough events, do not grab more
                if (SavedWaveforms >= _doe.VBDData.DataPulses ) {
                    break;
                }

                // If the next even time is smaller than the previous
                // it means the trigger tag overflowed. We ignore it
                // because I do not feel like making the math right now
                if (_processing_evts[k]->Info.TriggerTimeTag <
                    _processing_evts[k - 1]->Info.TriggerTimeTag) {
                    continue;
                }

                auto dtsp = _processing_evts[k]->Info.TriggerTimeTag
                    - _processing_evts[k - 1]->Info.TriggerTimeTag;

                // 1 tsp = 8 ns as per CAEN documentation
                // dtsp * 8ns = time difference in ns
                // In other words, do not add events with a time
                // difference between the last pulse of 1000ns or less
                if (dtsp*8 < 1000) {
                    continue;
                }

                if (_pulse_file) {
                    // Copy event to the file buffer
                    _pulse_file->Add(_processing_evts[k]);
                    SavedWaveforms++;
                }
            }

            _indicator_sender(IndicatorNames::SAVED_WAVEFORMS, SavedWaveforms);
        };

        if (isFileOpen) {
            // spdlog::info("Saving SIPM data");
            save(_pulse_file, sbc_save_func, _caen_port);
        } else {
            // Get current date and time, e.g. 202201051103
            std::chrono::system_clock::time_point now
                = std::chrono::system_clock::now();
            std::time_t now_t = std::chrono::system_clock::to_time_t(now);
            char filename[16];
            std::strftime(filename, sizeof(filename), "%Y%m%d%H%M",
                std::localtime(&now_t));

            std::ostringstream out;
            out.precision(3);
            out << _doe.LatestTemperature << "degC_";
            out << _doe.SiPMVoltageSysVoltage << "V";
            file_name = _doe.RunDir
                + "/" + _run_name
                + "/" + std::to_string(_doe.SiPMID) + "_"
                + std::to_string(_doe.CellNumber) + "cell_"
                + out.str()
                + "_data.bin";
            open(_pulse_file,
                file_name,
                // + doe.SiPMParameters + ".bin",

                // sbc_init_file is a function that saves the header
                // of the sbc data format as a function of record length
                // and number of channels
                sbc_init_file,
                _caen_port);

            // Save when the file was opened
            double t = get_current_time_epoch() / 1000.0;
            _saveinfo_file->Add(SaveFileInfo(t, file_name));

            async_save(_saveinfo_file, [](const SaveFileInfo& val){
                return std::to_string(val.Time) + "," + val.FileName + "\n";
            });

            _indicator_sender(IndicatorNames::DONE_DATA_TAKING, false);

            isFileOpen = _pulse_file > 0;
            SavedWaveforms = 0;
        }

        process_events();
        extract_for_gui_nb();
        if (SavedWaveforms >= _doe.VBDData.DataPulses) {
            switch_state(CAENInterfaceStates::OscilloscopeMode);

            // Closing remarks
            double t = get_current_time_epoch() / 1000.0;
            _saveinfo_file->Add(SaveFileInfo(t, file_name));

            async_save(_saveinfo_file, [](const SaveFileInfo& val){
                return std::to_string(val.Time) + "," + val.FileName + "\n";
            });

            isFileOpen = false;
            _indicator_sender(IndicatorNames::DONE_DATA_TAKING, true);
        }

        if (change_state()) {
            isFileOpen = false;
            close(_pulse_file);
        }

        return true;
    }

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
        if (_caen_port->Data.DataSize <= 0) {
            return;
        }

        // For the GUI
        static std::default_random_engine generator;
        std::uniform_int_distribution<uint32_t>
            distribution(0, _caen_port->Data.NumEvents);
        uint32_t rdm_num = distribution(generator);

        extract_event(_caen_port, rdm_num, _osc_event);

        process_data_for_gui();
    }

    void process_data_for_gui() {
        calculate_trigger_frequency();
        for (std::size_t j = 0; j < _caen_port->ModelConstants.NumChannels; j++) {
            // auto& ch_num = ch.second.Number;
            auto buf = _osc_event->Data->DataChannel[j];
            auto size = _osc_event->Data->ChSize[j];

            if (size <= 0) {
                continue;
            }

            _x_values.resize(size);
            _y_values.resize(size);

            for (uint32_t i = 0; i < size; i++) {
                _x_values[i] = i*
                    (1e9/_caen_port->ModelConstants.AcquisitionRate);
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
            for(auto r : latest_values) {
                double time = r.Time;
                double volt = r.Volt;
                double curr = r.Current;

                volt = calculate_sipm_voltage(volt, curr);

                // This measure is a NaN/Overflow from the picoammeter
                if(curr < 9.9e37){
                    _latest_measure.Volt = volt;
                    _latest_measure.Time = time;
                    _latest_measure.Current = curr;
                    _voltages_file->Add(_latest_measure);
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
            case CAENInterfaceStates::BreakdownVoltageMode:
            case CAENInterfaceStates::RunMode:
                get_voltage();
                save_voltages();

                if (_doe.SiPMVoltageSysChange) {
                    _sipm_volt_sys.get([&](serial_ptr& port)
                    -> std::optional<SiPMVoltageMeasure> {
                        send_msg(port, "++addr 22\n", "");

                        if (_doe.SiPMVoltageSysSupplyEN) {
                            send_msg(port, ":sour:volt:stat ON\n", "");
                            spdlog::warn("Turning on voltage supply.");
                        } else {
                            send_msg(port, ":sour:volt:stat OFF\n", "");
                            spdlog::warn("Turning off voltage supply.");
                        }

                        send_msg(port, ":sour:volt "
                            + std::to_string(_doe.SiPMVoltageSysVoltage)
                            + "\n", "");

                        spdlog::info("Changing output voltage to {0}",
                            _doe.SiPMVoltageSysVoltage);

                        // Setting a delay when other functionalities can start
                        // working as normal
                        _reset_timer = 20000.0; // ms

                        return {};
                    });

                    // Reset
                    _doe.SiPMVoltageSysChange = false;
                }
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
