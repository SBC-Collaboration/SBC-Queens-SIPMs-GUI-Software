#ifndef ACQUISTIONROUTINE_H
#define ACQUISTIONROUTINE_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <string>
// C++ 3rd party includes
// my includes
#include "sbcqueens-gui/timing_events.hpp"
#include "sbcqueens-gui/caen_helper.hpp"
#include "sbcqueens-gui/file_helpers.hpp"

#include "sbcqueens-gui/hardware_helpers/CAENInterfaceData.hpp"

#include "sipmanalysis/GainVBDEstimation.hpp"

namespace SBCQueens {

class AcquisitionRoutine {
    // CAEN Digitizer resource
    CAEN& _caen_port;
    // Information about the current state of the SiPM/CAEN software
    const CAENInterfaceData& _doe;
    // Manager of the digitizer pulse file
    DataFile<CAENEvent> _pulse_file;
    // Logfile to hold all the important values calculated in this routine.
    LogFile _saveinfo_file;
    // Current run name
    std::string _run_name;
    // Variable to hold the current file name (and directory)
    std::string _file_name = "";

    // Latest number of Events acquired from the CAEN digitizer
    uint32_t _latest_num_events = 0;
    // Total number of Events acquired from the CAEN digitizer
    uint32_t _acq_pulses = 0;
    // Flag to indicate if there are new Events from the CAEN digitizer
    bool _has_new_events = false;
    // Register to the current voltage being applied to the SiPM
    const double* _current_overvoltage;
    // Flag to indicate if the voltage has been changed
    bool _has_voltage_changed = false;
    // Flag to indicate if the routine has finished
    bool _has_finished = false;

    // Buffer of the latest 1024 (max allowed) Events from the CAEN digitizer
    std::array<CAENEvent, 1024> _processing_evts;

    // Latest breakdown voltage routine values
    const GainVBDFitParameters _vbe_analysis_in;

    // Retrieves events from the CAEN digitizer, and extracts them to
    // _processing_evts
    auto _process_events() noexcept {
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
    // "{SIPMID}_{SIPMCELL}cell_{TEMP}degC_{OVER-VOLTAGE}OV_databin"
    void _open_sipm_file() noexcept {
        std::ostringstream out;
        out.precision(3);
        out << _doe.LatestTemperature << "degC_";
        out << *_current_overvoltage << "OV";
        _file_name = _doe.RunDir
            + "/" + _run_name
            + "/" + std::to_string(_doe.SiPMID) + "_"
            + std::to_string(_doe.CellNumber) + "cell_"
            + out.str()
            + "_data.bin";

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
        _current_overvoltage = OverVoltages.cbegin();
        _has_voltage_changed = true;
    }

 public:

    const static inline std::array<double, 7> OverVoltages = {
        2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0 };

        // Takes ownership of the port and shared the saveinfo data
    AcquisitionRoutine(CAEN& port, CAENInterfaceData& doe,
        const std::string& runName, LogFile svInfoFile,
        const GainVBDFitParameters& vbe) :
        _caen_port{port},
        _doe{doe},
        _saveinfo_file{svInfoFile},
        _run_name{runName},
        _current_overvoltage{OverVoltages.cbegin()},
        _vbe_analysis_in{vbe}
    {
        if (not _caen_port) {
            throw "AcquistionRoutine cannot be created with an empty CAEN "
            "resource";
        }

        if (not _saveinfo_file) {
            throw "AcquistionRoutine cannot be created with an empty file "
            "resource";
        }

        std::generate(_processing_evts.begin(), _processing_evts.end(),
        [&](){
            return std::make_shared<caenEvent>(_caen_port->Handle);
        });
    }

    ~AcquisitionRoutine() {
        _close_sipm_file();
    }

    // Updates the acquisition
    bool update();

    // Latest number of Events acquired from the CAEN digitizer
    auto getLatestNumEvents() noexcept {
        return _latest_num_events;
    }

    // Total number of Events acquired from the CAEN digitizer
    auto getTotalAcquiredEvents() noexcept {
        return _acq_pulses;
    }

    // Returns a copy to the internal flag used to indicate if the voltage
    // needs to be changed.
    auto hasVoltageChanged() noexcept {
        return _has_voltage_changed;
    }

    // Returns the routine current voltage.
    auto getCurrentVoltage() noexcept {
        return *_current_overvoltage + _vbe_analysis_in.BreakdownVoltage;
    }

    auto hasFinished() noexcept {
        return _has_finished;
    }

    auto getLatestEvents() noexcept {
        return _processing_evts;
    }

    // Retrieves the CAEN resource. If not retrieved, this class will
    // free the resources when out of scope.
    // CAEN retrieveCAEN() noexcept {
    //     return std::move(_caen_port);
    // }

    // Resets the internal state and resets the routine.
    // void reset() noexcept;
};

}  // namespace SBCQueens

#endif