#ifndef CAENDIGITIZERINTERFACE_H
#define CAENDIGITIZERINTERFACE_H
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

// C++ 3rd party includes
#include <armadillo>
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
#include "include/armadillo_helpers.hpp"

#include "include/HardwareHelpers/ClientController.hpp"
#include "include/HardwareHelpers/Calibration.hpp"

#include "sipmcharacterization/PulseFunctions.hpp"
#include "sipmcharacterization/SPEAnalysis.hpp"
#include "sipmcharacterization/GainVBDEstimation.hpp"

namespace SBCQueens {

struct SaveFileInfo {
    double Time;
    std::string FileName;

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
    // In
    uint32_t SPEEstimationTotalPulses = 10000;
    uint32_t GainCalculationSPEPulses = 1000;
    uint32_t DataPulses = 1000000;

    VBRState State = VBRState::Idle;
    // // Out
    // double SPEEfficiency = 0.0;
    // double CalculatedBreakdownVoltage = 0.0;
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
    bool ResetCaen = false;

    std::string SiPMVoltageSysPort = "";
    double LatestTemperature;
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
    CAENInterfaceData doe;
    IndicatorSender<IndicatorNames> _indicatorSender;
    IndicatorSender<uint8_t> _plotSender;

    // Files
    DataFile<CAENEvent> _pulseFile;
    DataFile<SiPMVoltageMeasure> _voltagesFile;
    DataFile<SaveFileInfo> _saveinfoFile;

    // Hardware
    CAEN Port = nullptr;
    ClientController<serial_ptr, std::pair<double, double>> SiPMVoltageSystem;

    SiPMVoltageMeasure latest_measure;
    uint64_t SavedWaveforms = 0;
    uint64_t TriggeredWaveforms = 0;

    // tmp stuff
    uint16_t* data;
    size_t length;
    std::vector<double> x_values, y_values;
    CAENEvent osc_event, adj_osc_event;
    // 1024 because the caen can only hold 1024 no matter what model (so far)
    CAENEvent processing_evts[1024];

    // Analysis
    arma::mat coords;
      // coords(0, 0) = 100;
      // coords(1, 0) = 35e3;
      // coords(2, 0) = 20;
      // coords(3, 0) = 5;
      // coords(4, 0) = 7500;
    std::unique_ptr< SPEAnalysis<nEXOSiPMFunction> > spe_analysis;
    std::unique_ptr< GainVBDEstimation > vbe_analysis;
    // SPEAnalysis<nEXOSiPMFunction> _speanalysis(
    // 100, 1100, coords
    // );

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
        _indicatorSender(std::get<GeneralIndicatorQueue&>(_queues)),
        _plotSender(std::get<SiPMPlotQueue&>(_queues)),
        SiPMVoltageSystem("Keithley 2000/6487") {
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

        coords = arma::mat(5, 1, arma::fill::ones);
    }

    // No copying
    CAENDigitizerInterface(const CAENDigitizerInterface&) = delete;

    ~CAENDigitizerInterface() {}

    void operator()() {
        spdlog::info("Initializing CAEN thread");

        main_loop_state = standby_state;
        vbe_analysis.reset(new GainVBDEstimation());

        SiPMVoltageSystem.init(
            [=](serial_ptr& port) -> bool {
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
                connect_par(port, doe.SiPMVoltageSysPort, ssp);

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
                    + std::to_string(doe.SiPMVoltageSysVoltage));
                send_msg_slow(":sour:volt:stat OFF");

                // Ammeter side
                send_msg_slow(":sens:curr:dc:nplc 6");
                send_msg_slow(":sens:aver:stat off");

                // Zero check
                send_msg_slow(":syst:zch ON");
                send_msg_slow(":curr:rang 2E-4");
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

                // send_msg_slow("++auto 1");

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
                if (port) {
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
        _indicatorSender(IndicatorNames::TRIGGERRATE, dWaveforms / dt);
    }

    // Local error checking. Checks if there was an error and prints it
    // using spdlog
    bool lec() {
        return check_error(Port, [](const std::string& cmd) {
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
            if (!task(doe)) {
                spdlog::warn("Something went wrong with a command!");
            } else {
                switch_state(doe.CurrentState);
                return true;
            }
        }

        return false;
    }

    // Logic to disconnect the CAEN and clear up memory
    void close_caen() {
        for (CAENEvent& evt : processing_evts) {
            if (evt) {
                evt.reset();
            }
        }

        if (osc_event) {
            osc_event.reset();
        }

        if (adj_osc_event) {
            adj_osc_event.reset();
        }

        auto err = disconnect(Port);
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
    // starts acquisition, and moves the oscilloscope mode
    bool attempt_connection() {
        // If the port is open, clean up and close the port.
        // This will help turn attempt_connection into a reset
        // for the entire system.
        if (Port) {
            close_caen();
        }

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

        setup(Port, doe.GlobalConfig, doe.GroupConfigs);

        // Enable acquisition HAS to be called AFTER setup
        enable_acquisition(Port);

        // Trying to call once will initialize the port
        // but really this call is not important
        // SiPMVoltageSystem.get([=](serial_ptr& port) -> std::optional<double> {
        //     return {};
        // });

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

        open(_saveinfoFile, doe.RunDir + "/" + doe.RunName + "SaveInfo.txt");
        bool s = (_saveinfoFile != nullptr);

        if (!s) {
            spdlog::error("Failed to open SaveInfo.txt");
            switch_state(CAENInterfaceStates::Disconnected);
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
            TriggeredWaveforms += Port->Data.NumEvents;
            // spdlog::info("Total size buffer: {0}",  Port->Data.TotalSizeBuffer);
            // spdlog::info("Data size: {0}", Port->Data.DataSize);
            // spdlog::info("Num events: {0}", Port->Data.NumEvents);

            extract_event(Port, 0, osc_event);


            // spdlog::info("Event size: {0}", osc_event->Info.EventSize);
            // spdlog::info("Event counter: {0}", osc_event->Info.EventCounter);
            // spdlog::info("Trigger Time Tag: {0}",
            //     osc_event->Info.TriggerTimeTag);

            process_data_for_gui();

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
    bool breakdown_voltage_mode() {
        static uint32_t num_events = 0;
        static auto process_events = [&]() {
            bool isData = retrieve_data_until_n_events(Port,
                Port->GlobalConfig.MaxEventsPerRead);

            // While all of this is happening, the digitizer is taking data
            if (!isData) {
                return;
            }

            // double frequency = 0.0;
            num_events = Port->Data.NumEvents;
            TriggeredWaveforms += num_events;
            for (uint32_t i = 0; i < num_events; i++) {
                // Extract event i
                extract_event(Port, i, processing_evts[i]);
            }
        };

        process_events();
        //SavedWaveforms += Port->Data.NumEvents;
        // startup!
        static arma::mat spe_estimation_pulse_buffer;
        static uint32_t acq_pulses = 0;
        static bool init_done = false;
        static bool spe_estimation_done = false;
        switch (doe.VBDData.State) {
            case VBRState::Init:
            {
                if (!init_done) {
                    // Memory allocations and prepare the resources for whenever
                    // the user presses the next button, and prepares the file

                    // File preparation
                    std::ostringstream out;
                    out.precision(1);
                    out << doe.LatestTemperature;
                    std::string fileName = doe.RunDir
                        + "/" + doe.RunName
                        + "/" + doe.SiPMName + out.str() + "K_"
                        + std::to_string(doe.SiPMVoltageSysVoltage) + "V"
                        + "_spe_estimation.bin";

                    open(_pulseFile,
                        fileName,
                        // sbc_init_file is a function that saves the header
                        // of the sbc data format as a function of record length
                        // and number of channels
                        sbc_init_file,
                        Port);

                    double t = get_current_time_epoch();
                    _saveinfoFile->Add(SaveFileInfo(t, fileName));

                    // It will hold this many pulses so let's allocate what we need
                    spe_estimation_pulse_buffer.resize(
                        doe.VBDData.SPEEstimationTotalPulses,
                        doe.GlobalConfig.RecordLength);

                    double prepulse_end_region_per =
                        1.0 - 0.01*doe.GlobalConfig.PostTriggerPorcentage;
                    int32_t prepulse_end_region
                        = static_cast<int32_t>(
                            doe.GlobalConfig.RecordLength*prepulse_end_region_per)
                        - 70;  // the 70 is a constant delay in the trigger

                    uint32_t prepulse_end_region_u = prepulse_end_region < 0 ?
                        0 : static_cast<uint32_t>(prepulse_end_region);

                    coords(0, 0) = prepulse_end_region + 1;
                    coords(1, 0) = 35e3;
                    coords(3, 0) = v_threshold_cts_to_adc_cts(Port,
                        doe.GroupConfigs[0].DCOffset);
                    coords(3, 0) = 20;
                    coords(4, 0) = 5;

                    spdlog::info("Prepulse_end_region: {0}",
                        prepulse_end_region_u);

                    // We create the analysis object with all the info, but
                    // this does not do the analysis!
                    spe_analysis.reset(new SPEAnalysis<nEXOSiPMFunction>(
                        prepulse_end_region_u,
                        doe.GlobalConfig.RecordLength, coords));

                    acq_pulses = 0;
                    init_done = true;
                    SavedWaveforms = 0;

                    // Front end preparation
                    _indicatorSender(
                        IndicatorNames::FULL_ANALYSIS_DONE, false);
                    _indicatorSender(
                        IndicatorNames::CALCULATING_VBD, false);

                    doe.VBDData.State = VBRState::Analysis;
                } else {
                    doe.VBDData.State = VBRState::Idle;
                }
            }
            break;
            case VBRState::Analysis:
                // k = 1, we always throw the first event
                for (uint32_t k = 1; k < num_events; k++) {

                    // If we already have enough events, do not grab more
                    if (acq_pulses >= doe.VBDData.SPEEstimationTotalPulses ) {
                        break;
                    }

                    // If the next even time is smaller than the previous
                    // it means the trigger tag overflowed. We ignore it
                    // because I do not feel like making the math right now
                    if ( processing_evts[k]->Info.TriggerTimeTag <
                        processing_evts[k - 1]->Info.TriggerTimeTag) {
                        continue;
                    }

                    auto dtsp = processing_evts[k]->Info.TriggerTimeTag
                        - processing_evts[k - 1]->Info.TriggerTimeTag;

                    // 1 tsp = 8 ns as per CAEN documentation
                    // dtsp * 8ns = time difference in ns
                    // In other words, do not add events with a time
                    // difference between the last pulse of 1000ns
                    if(dtsp*8 < 1000) {
                        continue;
                    }

                    if (_pulseFile) {
                        _pulseFile->Add(processing_evts[k]);
                    }

                    spe_estimation_pulse_buffer.row(acq_pulses)
                        = caen_event_to_armadillo(processing_evts[k], 1);

                    acq_pulses++;
                }

                // This is to update the indicator
                SavedWaveforms = acq_pulses;
                save(_pulseFile, sbc_save_func, Port);
                if (acq_pulses == doe.VBDData.SPEEstimationTotalPulses) {
                    spe_estimation_done = true;

                    // This is the line of code that runs the analysis!
                    auto pars = spe_analysis->FullAnalysis(
                        spe_estimation_pulse_buffer);

                    _indicatorSender(IndicatorNames::SPE_GAIN_MEAN,
                        pars.SPEParameters(1));

                    _indicatorSender(IndicatorNames::WAVEFORM_NOISE,
                        pars.BaselineParameters.NoiseSTD);

                    _indicatorSender(IndicatorNames::WAVEFORM_BASELINE,
                        pars.BaselineParameters.Baseline);

                    _indicatorSender(IndicatorNames::SPE_EFFICIENCTY,
                        pars.SPEEfficiency);

                    _indicatorSender(IndicatorNames::INTEGRAL_THRESHOLD,
                        pars.IntegralThreshold);

                    _indicatorSender(IndicatorNames::RISE_TIME,
                        pars.SPEParameters(3));

                    _indicatorSender(IndicatorNames::FALL_TIME,
                        pars.SPEParameters(4));

                    _indicatorSender(IndicatorNames::OFFSET,
                        pars.SPEParameters(2));

                    _indicatorSender(IndicatorNames::FULL_ANALYSIS_DONE, true);

                    if (pars.SPEEfficiency != 0.0) {
                        _indicatorSender(IndicatorNames::GAIN_VS_VOLTAGE,
                            latest_measure.Volt,
                            pars.SPEParameters(1));

                        vbe_analysis->add(pars.SPEParameters(1),
                            pars.SPEParametersErrors(1),
                            latest_measure.Volt);

                        double t = get_current_time_epoch();

                        for (arma::uword i = 0; i < pars.SPEParameters.n_elem; i++) {
                            _saveinfoFile->Add(
                                SaveFileInfo(t,
                            std::to_string(pars.SPEParameters(i))));

                            _saveinfoFile->Add(
                                SaveFileInfo(t,
                            std::to_string(pars.SPEParametersErrors(i))));
                        }
                    }

                    doe.VBDData.State = VBRState::Idle;
                }

            break;
            case VBRState::CalculateBreakdownVoltage:
            {
                if (vbe_analysis->size() >= 3) {
                    auto values = vbe_analysis->calculate();

                    if (values.BreakdownVoltageError != 0.0) {
                        _indicatorSender(IndicatorNames::BREAKDOWN_VOLTAGE,
                            values.BreakdownVoltage);
                        _indicatorSender(IndicatorNames::BREAKDOWN_VOLTAGE_ERR,
                            values.BreakdownVoltageError);
                        _indicatorSender(IndicatorNames::CALCULATING_VBD, true);
                    }
                }

                doe.VBDData.State = VBRState::Idle;
            }
            break;
            case VBRState::ResetAll:
                _indicatorSender(IndicatorNames::CALCULATING_VBD, false);
                vbe_analysis.reset(new GainVBDEstimation());
            case VBRState::Reset:
                close(_pulseFile);
                init_done = false;
                spe_estimation_done = false;
                SavedWaveforms = 0;
                acq_pulses = 0;
                doe.VBDData.State = VBRState::Idle;
                _indicatorSender(IndicatorNames::FULL_ANALYSIS_DONE, false);
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

        _indicatorSender(IndicatorNames::SAVED_WAVEFORMS,
                    SavedWaveforms);
        checkerror();
        extract_for_gui_nb();
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

            for (uint32_t k = 0; k < Port->Data.NumEvents; k++) {
                // Extract event i
                extract_event(Port, k, processing_evts[k]);
            }

            for (uint32_t k = 1; k < Port->Data.NumEvents; k++) {
                          // If we already have enough events, do not grab more
                if (SavedWaveforms >= doe.VBDData.DataPulses ) {
                    break;
                }

                // If the next even time is smaller than the previous
                // it means the trigger tag overflowed. We ignore it
                // because I do not feel like making the math right now
                if ( processing_evts[k]->Info.TriggerTimeTag <
                    processing_evts[k - 1]->Info.TriggerTimeTag) {
                    continue;
                }

                auto dtsp = processing_evts[k]->Info.TriggerTimeTag
                    - processing_evts[k - 1]->Info.TriggerTimeTag;

                // 1 tsp = 8 ns as per CAEN documentation
                // dtsp * 8ns = time difference in ns
                // In other words, do not add events with a time
                // difference between the last pulse of 1000ns or less
                if (dtsp*8 < 1000) {
                    continue;
                }

                if (_pulseFile) {
                    // Copy event to the file buffer
                    _pulseFile->Add(processing_evts[k]);
                    SavedWaveforms++;
                }
            }

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

            std::ostringstream out;
            out.precision(1);
            out << doe.LatestTemperature;
            std::string fileName = doe.RunDir
                + "/" + doe.RunName
                + "/" + doe.SiPMName + out.str() + "K_"
                + std::to_string(doe.SiPMVoltageSysVoltage) + "V"
                + "_data.bin";
            open(_pulseFile,
                fileName,
                // + doe.SiPMParameters + ".bin",

                // sbc_init_file is a function that saves the header
                // of the sbc data format as a function of record length
                // and number of channels
                sbc_init_file,
                Port);

            double t = get_current_time_epoch();
            _saveinfoFile->Add(SaveFileInfo(t, fileName));

            isFileOpen = _pulseFile > 0;
            SavedWaveforms = 0;
        }

        process_events();
        extract_for_gui_nb();
        if (SavedWaveforms >= doe.VBDData.DataPulses ) {
            switch_state(CAENInterfaceStates::OscilloscopeMode);
        }

        if (change_state()) {
            isFileOpen = false;
            close(_pulseFile);
        }

        return true;
    }

    bool disconnected_mode() {
        spdlog::warn("Manually losing connection to the CAEN digitizer.");
        close_caen();
        switch_state(CAENInterfaceStates::Standby);
        return true;
    }

    bool closing_mode() {
        spdlog::info("Going to close the CAEN thread.");
        close_caen();
        return false;
    }

    void rdm_extract_for_gui() {
        if (Port->Data.DataSize <= 0) {
            return;
        }

        // For the GUI
        static std::default_random_engine generator;
        std::uniform_int_distribution<uint32_t>
            distribution(0, Port->Data.NumEvents);
        uint32_t rdm_num = distribution(generator);

        extract_event(Port, rdm_num, osc_event);

        process_data_for_gui();
    }

    void process_data_for_gui() {
        calculate_trigger_frequency();
        for (std::size_t j = 0; j < Port->ModelConstants.NumChannels; j++) {
            // auto& ch_num = ch.second.Number;
            auto buf = osc_event->Data->DataChannel[j];
            auto size = osc_event->Data->ChSize[j];

            if (size <= 0) {
                continue;
            }

            x_values.resize(size);
            y_values.resize(size);

            for (uint32_t i = 0; i < size; i++) {
                x_values[i] = i*(1.0/Port->ModelConstants.AcquisitionRate)*(1e9);
                y_values[i] = static_cast<double>(buf[i]);
            }

            _plotSender(static_cast<uint8_t>(j),
                x_values,
                y_values);
        }
    }

    void sipm_voltage_system_update() {
        static auto getVoltage = make_total_timed_event(
            std::chrono::seconds(1),
            [&]() {
                auto r = SiPMVoltageSystem.get(
                [=](serial_ptr& port)
                -> std::optional<std::pair<double, double>> {
                    static auto send_msg_slow = make_blocking_total_timed_event(
                        std::chrono::milliseconds(10),
                        [&](const std::string& msg){
                            // spdlog::info("Sending msg to Keithely: {0}", msg);
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

                    return std::make_optional(
                        std::make_pair(volt.value(), curr.value()));
            });

            if (r) {
                double time = get_current_time_epoch() / 1000.0;
                double volt = r.value().first;
                double curr = r.value().second;

                volt = calculate_sipm_voltage(volt, curr);

                // This measure is a NaN/Overflow from the picoammeter
                if(curr < 9.9e37){
                    latest_measure.Volt = volt;
                    latest_measure.Time = time;
                    latest_measure.Current = curr;
                    _voltagesFile->Add(latest_measure);
                    _indicatorSender(IndicatorNames::DMM_VOLTAGE, time, volt);
                    _indicatorSender(IndicatorNames::LATEST_DMM_VOLTAGE, volt);
                    _indicatorSender(IndicatorNames::PICO_CURRENT, time, curr);
                    _indicatorSender(IndicatorNames::LATEST_PICO_CURRENT, curr);
                }
            }
        });

        static auto saveVoltages = make_total_timed_event(
            std::chrono::seconds(30),
            [&]() {
                spdlog::info("Saving voltage (Keithley 2000) voltages.");
                async_save(_voltagesFile,
                [](const SiPMVoltageMeasure& mes) {
                    std::ostringstream out;
                    out.precision(7);
                    out << "," << mes.Volt <<
                        "," << mes.Current << "\n" << std::scientific;;

                    return  std::to_string(mes.Time) + out.str();
            });
        });

        switch (doe.CurrentState) {
            case CAENInterfaceStates::OscilloscopeMode:
            case CAENInterfaceStates::BreakdownVoltageMode:
            case CAENInterfaceStates::RunMode:
                getVoltage();
                saveVoltages();
            {
                if (doe.SiPMVoltageSysChange) {
                    SiPMVoltageSystem.get([=](serial_ptr& port)
                    -> std::optional<std::pair<double, double>> {
                        send_msg(port, "++addr 22\n", "");

                        if(doe.SiPMVoltageSysSupplyEN) {
                            send_msg(port, ":sour:volt:stat ON\n", "");
                            spdlog::info("Turning on voltage supply");
                        } else {
                            send_msg(port, ":sour:volt:stat OFF\n", "");
                            spdlog::warn("Turning off voltage supply");
                        }

                        send_msg(port, ":sour:volt "
                            + std::to_string(doe.SiPMVoltageSysVoltage)
                            + "\n", "");

                        spdlog::info("Changing output voltage to {0}",
                            doe.SiPMVoltageSysVoltage);

                        // send_msg(port, "++auto 1\n", "");
                        return {};
                    });

                    // Reset
                    doe.SiPMVoltageSysChange = false;
                }
            }
            break;

            case CAENInterfaceStates::Standby:
            case CAENInterfaceStates::AttemptConnection:
            case CAENInterfaceStates::Disconnected:
            break;
            case CAENInterfaceStates::Closing:
                SiPMVoltageSystem.get([=](serial_ptr& port)
                    -> std::optional<std::pair<double, double>> {
                        send_msg(port, "++addr 22\n", "");
                        send_msg(port, ":sour:volt:stat OFF\n", "");
                        send_msg(port, ":sour:volt 0.0\n", "");
                        return {};
                    });
                SiPMVoltageSystem.close();
                spdlog::warn("Closing DMM port");
            case CAENInterfaceStates::NullState:
            default:
            break;
        }
    }
};
}  // namespace SBCQueens
#endif
