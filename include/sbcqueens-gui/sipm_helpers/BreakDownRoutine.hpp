#ifndef BREAKDOWNROUTINE_H
#define BREAKDOWNROUTINE_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <string>
// C++ 3rd party includes
// my includes
#include "sbcqueens-gui/caen_helper.hpp"
#include "sbcqueens-gui/file_helpers.hpp"

#include "sbcqueens-gui/hardware_helpers/CAENInterfaceData.hpp"

#include "sipmanalysis/PulseFunctions.hpp"
#include "sipmanalysis/SPEAnalysis.hpp"
#include "sipmanalysis/GainVBDEstimation.hpp"
#include "sipmanalysis/StabilityTools.hpp"

namespace SBCQueens {

enum class BreakDownRoutineState {
    Unusued = 0,
    Idle,
    Init,
    Analysis,
    CalculateBreakdownVoltage,
    SoftReset,
    HardReset,
    Finished
};

class BreakDownRoutine {
    CAEN _caen_port;
    const CAENInterfaceData& _doe;
    DataFile<CAENEvent> _pulse_file;
    LogFile _saveinfo_file;
    std::string _run_name;
    std::string _file_name = "";

    BreakDownRoutineState _current_state = BreakDownRoutineState::Init;
    uint32_t _latest_num_events = 0;
    uint32_t _acq_pulses = 0;
    bool _new_events = false;

    const double* _current_voltage;
    bool _is_voltage_changed = false;
    bool _is_new_gain_measurement = false;

    std::array<CAENEvent, 1024> _processing_evts;
    arma::mat _coords;
    arma::mat _spe_estimation_pulse_buffer;
    std::unique_ptr< SPEAnalysis<SimplifiedSiPMFunction> > _spe_analysis;
    std::unique_ptr< GainVBDEstimation > _vbe_analysis;

    SPEAnalysisParams<SimplifiedSiPMFunction> _spe_analysis_out;
    GainVBDFitParameters _vbe_analysis_out;

    // Retrieves events, and extracts them,
    auto _process_events() {
        bool is_new_data = retrieve_data_until_n_events(_caen_port,
            _caen_port->GlobalConfig.MaxEventsPerRead);

        // While all of this is happening, the digitizer is taking data
        if (not is_new_data) {
            return false;
        }

        // double frequency = 0.0;
        _latest_num_events = _caen_port->Data.NumEvents;
        for (uint32_t i = 0; i < _latest_num_events; i++) {
            // Extract event i
            extract_event(_caen_port, i, _processing_evts[i]);
        }

        return true;
    };

    void _open_sipm_file() {
        std::ostringstream out;
        out.precision(3);
        out << _doe.LatestTemperature << "degC_";
        out << *_current_voltage << "V";
        _file_name = _doe.RunDir
            + "/" + _run_name
            + "/" + std::to_string(_doe.SiPMID) + "_"
            + std::to_string(_doe.CellNumber) + "cell_"
            + out.str()
            + "_spe_estimation.bin";

        open(_pulse_file,
            _file_name,
            // sbc_init_file is a function that saves the header
            // of the sbc data format as a function of record length
            // and number of channels
            sbc_init_file,
            _caen_port);
    }

    void _reset_voltage() {
        _current_voltage = GainVoltages.cbegin();
        _is_voltage_changed = true;
    }

    bool _idle();
    bool _init();
    bool _analysis();
    bool _calculate_breakdown_voltage();
    bool _finished();
    bool _soft_reset();
    bool _hard_reset();

 public:
    // These are SiPM dependent but for now we are aiming at VUV4!
    const static inline std::array<double, 3> GainVoltages = {
        52.0, 53.0, 54.0 };

    // Takes ownership of the port and shared the saveinfo data
    BreakDownRoutine(CAEN port, CAENInterfaceData& doe,
        const std::string& runName, LogFile svInfoFile)
        : _caen_port{std::move(port)},
        _doe{doe},
        _saveinfo_file{svInfoFile},
        _run_name{runName},
        _current_voltage{GainVoltages.cbegin()}
    {
        std::generate(_processing_evts.begin(), _processing_evts.end(),
        [&](){
            return std::make_shared<caenEvent>(_caen_port->Handle);
        });
    }

    // Updates the state machine and the data.
    bool update();

    // Getters
    BreakDownRoutineState getCurrentState() const {
        return _current_state;
    }

    auto getLatestNumEvents() const {
        return _latest_num_events;
    }

    auto getTotalAcquiredEvents() const {
        return _acq_pulses;
    }

    auto hasVoltageChanged() const {
        return _is_voltage_changed;
    }

    auto getCurrentVoltage() const {
        return *_current_voltage;
    }

    auto hasNewGainMeasurement() const {
        return _is_new_gain_measurement;
    }

    SPEAnalysisParams<SimplifiedSiPMFunction> getAnalysisLatestValues() {
        return _spe_analysis_out;
    }

    GainVBDFitParameters getBreakdownVoltage() {
        return _vbe_analysis_out;
    }

    CAEN retrieveCAEN() {
        return std::move(_caen_port);
    }

    void reset() {
        _hard_reset();
    }
};


}  // namespace SBCQueens

#endif