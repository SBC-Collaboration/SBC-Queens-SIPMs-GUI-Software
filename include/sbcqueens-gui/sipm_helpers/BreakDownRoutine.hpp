#ifndef BREAKDOWNROUTINE_H
#define BREAKDOWNROUTINE_H
#include <cstddef>
#include <stdexcept>
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <string>
#include <span>
// C++ 3rd party includes
// my includes
#include "sbcqueens-gui/timing_events.hpp"
#include "sbcqueens-gui/caen_helper.hpp"
#include "sbcqueens-gui/file_helpers.hpp"

#include "sbcqueens-gui/hardware_helpers/CAENInterfaceData.hpp"

#include "sipmanalysis/PulseFunctions.hpp"
#include "sipmanalysis/SPEAnalysis.hpp"
#include "sipmanalysis/GainVBDEstimation.hpp"
#include "sipmanalysis/StabilityTools.hpp"

namespace SBCQueens {

enum class BreakdownRoutineState {
    Init,
    GainMeasurements,
    CalculateBreakdownVoltage,
    SoftReset,
    HardReset,
    Acquisition,
    Finished
};

// Does not manage voltage resources.
class BreakdownRoutine {
    // CAEN Digitizer resource
    CAEN& _caen_port;
    // Information about the current state of the SiPM/CAEN software
    CAENInterfaceData& _doe;
    // Manager of the digitizer pulse file
    DataFile<CAENEvent> _pulse_file = nullptr;
    // Logfile to hold all the important values calculated in this routine.
    LogFile _saveinfo_file = nullptr;
    // Current run name
    std::string _run_name = "";
    // Variable to hold the current file name (and directory)
    std::string _file_name = "";

    // Current State of the internal state-machine
    BreakdownRoutineState _current_state = BreakdownRoutineState::Init;

    // Latest number of Events acquired from the CAEN digitizer
    uint32_t _latest_num_events = 0;
    // Total number of Events acquired from the CAEN digitizer
    uint32_t _acq_pulses = 0;
    // Flag to indicate if there are new Events from the CAEN digitizer
    bool _has_new_events = false;
    // Register to the current voltage being applied to the SiPM
    const double* _current_voltage;
    // Flag to indicate if the voltage has been changed
    bool _has_voltage_changed = false;
    // Flag to indicate if there is a new gain measurement
    bool _has_new_gain_measurement = false;
    // Flag to indicate if there is a new breakdown voltage.
    bool _has_new_breakdown_voltage = false;

    // Buffer of the latest 1024 (max allowed) Events from the CAEN digitizer
    std::array<CAENEvent, 1024> _processing_evts;
    arma::mat _coords;
    // Buffer of the latest Events from the CAEN digitizer as a Armadillo mat
    arma::mat _spe_estimation_pulse_buffer;
    // Class that holds the routine to extract the gain (and other) parameters
    // from data.
    std::unique_ptr< SPEAnalysis<SimplifiedSiPMFunction> > _spe_analysis = nullptr;
    // Class that holds the routine to calculate the breakdown voltage from
    // the gains calculated from the _spe_analysis.
    std::unique_ptr< GainVBDEstimation > _vbe_analysis = nullptr;

    // Latest 1SPE values
    SPEAnalysisParams<SimplifiedSiPMFunction> _spe_analysis_out;
    // Latest breakdown voltage routine values
    GainVBDFitParameters _vbe_analysis_out;

    // Retrieves events from the CAEN digitizer, and extracts them to
    // _processing_evts
    bool _process_events() noexcept {
        bool is_new_data = retrieve_data_until_n_events(_caen_port,
            _caen_port->GlobalConfig.MaxEventsPerRead);

        if (not is_new_data) {
            return false;
        }

        _latest_num_events = _caen_port->Data.NumEvents;
        for (uint32_t i = 0; i < _latest_num_events; i++) {
            // Extract event i
            extract_event(_caen_port, i, _processing_evts[i]);
        }

        return true;
    };

    // Opens the SiPM pulse file given the current temperature, voltage,
    // SiPM ID, and cell.
    // The output file is named
    // "{SIPMID}_{SIPMCELL}cell_{TEMP}degC_{VOLT}V_spe_estimation.bin"
    void _open_sipm_file() noexcept {
        std::ostringstream out;
        if (_current_state == BreakdownRoutineState::Acquisition) {
            out.precision(3);
            out << _doe.LatestTemperature << "degC_";
            out << *_current_voltage << "OV";
            _file_name = _doe.RunDir
                + "/" + _run_name
                + "/" + std::to_string(_doe.SiPMID) + "_"
                + std::to_string(_doe.CellNumber) + "cell_"
                + out.str()
                + "_data.bin";
        } else {
            out.precision(3);
            out << _doe.LatestTemperature << "degC_";
            out << *_current_voltage << "V";
            _file_name = _doe.RunDir
                + "/" + _run_name
                + "/" + std::to_string(_doe.SiPMID) + "_"
                + std::to_string(_doe.CellNumber) + "cell_"
                + out.str()
                + "_spe_estimation.bin";
        }

        open(_pulse_file,
            _file_name,
            // sbc_init_file is a function that saves the header
            // of the sbc data format as a function of record length
            // and number of channels
            sbc_init_file,
            _caen_port);

        double t = get_current_time_epoch() / 1000.0;
        _saveinfo_file->Add(SaveFileInfo(t, _file_name));
    }

    // Closes the pulse file, and write to the logfile when it was closed.
    void _close_sipm_file() noexcept {
        if (not _pulse_file) {
            return;
        }

        // Close and open the next file
        close(_pulse_file);

        double t = get_current_time_epoch() / 1000.0;
        _saveinfo_file->Add(SaveFileInfo(t, _file_name));
    }

    // Resets the voltage and set the hasVoltageChanged() to true.
    void _reset_voltage() noexcept {
        if (_current_state == BreakdownRoutineState::Acquisition) {
            _current_voltage = OverVoltages.cbegin();
        } else {
            _current_voltage = GainVoltages.cbegin();
        }
        _has_voltage_changed = true;
    }

    template<typename Func>
    void filter_data(const std::size_t& n, Func&& f) noexcept {
        for (uint32_t k = 1; k < _latest_num_events; k++) {
            // If we already have enough events, do not grab more
            if (_acq_pulses >= n) {
                continue;
            }

            // If the next even time is smaller than the previous
            // it means the trigger tag overflowed. We ignore it
            // because I do not feel like making the math right now
            if (_processing_evts[k]->Info.TriggerTimeTag <
                 _processing_evts[k-1]->Info.TriggerTimeTag) {
                continue;
            }

            auto dtsp = _processing_evts[k]->Info.TriggerTimeTag
                -  _processing_evts[k-1]->Info.TriggerTimeTag;

            // 1 tsp = 8 ns as per CAEN documentation
            // dtsp * 8ns = time difference in ns
            // In other words, do not add events with a time
            // difference between the last pulse of 1000ns
            if (dtsp*8 < 10000) {
                continue;
            }

            _pulse_file->Add(_processing_evts[k]);

            f(_processing_evts[k]);

            _acq_pulses++;
        }
    }

    bool _idle() noexcept;
    bool _init() noexcept;
    bool _gain_measurements() noexcept;
    bool _calculate_breakdown_voltage() noexcept;
    bool _finished() noexcept;
    bool _soft_reset() noexcept;
    bool _hard_reset() noexcept;
    bool _acquistion() noexcept;

 public:
    // These are SiPM and temperature dependent but for now,
    // we are aiming at VUV4 from -20degC to -40degC
    constexpr static std::array<double, 3> GainVoltages{
        52.0, 53.0, 54.0 };

    constexpr static std::array<double, 7> OverVoltages{
        2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0 };

    // Takes ownership of the port and shared the saveinfo data
    BreakdownRoutine(CAEN& port, CAENInterfaceData& doe,
        const std::string& runName, LogFile svInfoFile) :
        _caen_port{port},
        _doe{doe},
        _saveinfo_file{svInfoFile},
        _run_name{runName},
        _current_voltage{GainVoltages.cbegin()}
    {
        if (not _caen_port) {
            throw std::invalid_argument("BreakdownRoutine cannot be created "
            "with an empty CAEN resource");
        }

        if (not _saveinfo_file) {
            throw std::invalid_argument("BreakdownRoutine cannot be created "
            "with an empty file resource");
        }

        std::generate(_processing_evts.begin(), _processing_evts.end(),
        [&](){
            return std::make_shared<caenEvent>(_caen_port->Handle);
        });
    }

    ~BreakdownRoutine() {
        _close_sipm_file();
    }

    // Updates the state machine.
    bool update() noexcept;

    // Gets the latest internal state
    BreakdownRoutineState getCurrentState() noexcept {
        return _current_state;
    }

    // Latest number of Events acquired from the CAEN digitizer
    uint32_t getLatestNumEvents() noexcept {
        return _latest_num_events;
    }

    // Total number of Events acquired from the CAEN digitizer
    uint32_t getTotalAcquiredEvents() noexcept {
        return _acq_pulses;
    }

    // Returns a copy to the internal flag used to indicate if the voltage
    // needs to be changed.
    bool hasVoltageChanged() noexcept {
        return _has_voltage_changed;
    }

    // Returns the routine current voltage.
    double getCurrentVoltage() noexcept {
        if (_current_state == BreakdownRoutineState::Acquisition) {
            return *_current_voltage + _vbe_analysis_out.BreakdownVoltage;
        }

        return *_current_voltage;
    }

    // Returns a copy to the internal flag used to indicate if there is a
    // new gain measurement
    bool hasNewGainMeasurement() noexcept {
        return _has_new_gain_measurement;
    }

    // Returns the latest 1SPE parameters. If none, most of the internal
    // data values will be 0.
    SPEAnalysisParams<SimplifiedSiPMFunction> getAnalysisLatestValues() noexcept {
        return _spe_analysis_out;
    }

    // Returns the breakdown voltage If none, it equals to 0.
    GainVBDFitParameters getBreakdownVoltage() noexcept {
        return _vbe_analysis_out;
    }

    std::array<CAENEvent, 1024> getLatestEvents() noexcept {
        return _processing_evts;
    }

    bool hasNewBreakdownVoltage() noexcept {
        return _has_voltage_changed;
    }

    // Retrieves the CAEN resource. If not retrieved, this class will
    // free the resources when out of scope.
    // CAEN retrieveCAEN() noexcept {
    //     return std::move(_caen_port);
    // }

    // Resets the internal state and resets the routine.
    void reset() noexcept {
        _hard_reset();
    }
};


}  // namespace SBCQueens

#endif
