#include "sbcqueens-gui/sipm_helpers/BreakDownRoutine.hpp"

// C STD includes
// C 3rd party includes
// C++ STD includes
// C++ 3rd party includes
#include <armadillo>
#include <spdlog/spdlog.h>

// my includes
#include "sbcqueens-gui/armadillo_helpers.hpp"

namespace SBCQueens {

bool BreakdownRoutine::update() noexcept {
    _has_new_events = _process_events();
    _has_voltage_changed = false;
    _has_new_gain_measurement = false;

    switch (_current_state) {
    case BreakdownRoutineState::Init:
        return _init();
    break;

    case BreakdownRoutineState::Analysis:
        return _analysis();
    break;

    case BreakdownRoutineState::CalculateBreakdownVoltage:
        return _calculate_breakdown_voltage();
    break;

    case BreakdownRoutineState::Finished:
        return _finished();
    break;

    case BreakdownRoutineState::HardReset:
        return _hard_reset();
    break;

    case BreakdownRoutineState::SoftReset:
        return _soft_reset();
    break;

    // These do nothing for now.
    case BreakdownRoutineState::Idle:
    case BreakdownRoutineState::Unusued:
    default:
         return true;
    }
}


bool BreakdownRoutine::_idle() noexcept {
    return true;
}

bool BreakdownRoutine::_init() noexcept {
    // We start at the first voltage, so we reset it.
    _reset_voltage();

    // First, we open the file corresponding to the first voltage.
    _open_sipm_file();

    // It will hold this many pulses so let's allocate what we need
    _spe_estimation_pulse_buffer.resize(_doe.VBDData.SPEEstimationTotalPulses,
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
    auto t = get_current_time_epoch() / 1000.0;
    _saveinfo_file->Add(SaveFileInfo(t, "expected_t0 : "
        + std::to_string(prepulse_end_region_u + 1)));

    // _coords is the guesses for the analysis
    // 0 being the t0 guess
    _coords = arma::mat(5, 1, arma::fill::zeros);
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

    spdlog::info("Expected analysis t0: {0}", prepulse_end_region_u);

    // We limit the analysis to a window of 400sp to speed up
    // the calculations
    arma::uword window =
        _doe.GlobalConfig.RecordLength - prepulse_end_region_u;

    window = std::clamp(window, window, 400ull);

    // We also log the tf to keep a record of it
    _saveinfo_file->Add(SaveFileInfo(t, "window : " + std::to_string(window)));

    spdlog::info("Analysis window: {0}", window);

    // We create the analysis object with all the info, but
    // this does not do the analysis!
    _spe_analysis = std::make_unique<SPEAnalysis<SimplifiedSiPMFunction>>(
            prepulse_end_region_u, window, _coords);

    _acq_pulses = 0;

    async_save(_saveinfo_file, [](const SaveFileInfo& val){
        return std::to_string(val.Time) + "," + val.FileName
            + "\n";
    });

    _current_state = BreakdownRoutineState::Analysis;
    _vbe_analysis = std::make_unique<GainVBDEstimation>();

    return true;
}

bool BreakdownRoutine::_analysis() noexcept {
    // Do not take data if temp or voltage is not stable
    if (not _doe.isVoltageStabilized || not _doe.isTemperatureStabilized) {
        return true;
    }

    // This case can stall if no data is coming in, maybe
    // add a timeout later.
    if (!_has_new_events) {
        spdlog::warn("No new events in the buffer during analysis");
        return true;
    }

    // k = 1, we always throw the first event
    for (uint32_t k = 1; k < _latest_num_events; k++) {

        // If we already have enough events, do not grab more
        if (_acq_pulses >= _doe.VBDData.SPEEstimationTotalPulses) {
            continue;
        }

        auto current_event = _processing_evts[k];
        auto prev_event = _processing_evts[k-1];

        // If the next even time is smaller than the previous
        // it means the trigger tag overflowed. We ignore it
        // because I do not feel like making the math right now
        if (current_event->Info.TriggerTimeTag <
            prev_event->Info.TriggerTimeTag) {
            continue;
        }

        auto dtsp = current_event->Info.TriggerTimeTag
            - prev_event->Info.TriggerTimeTag;

        // 1 tsp = 8 ns as per CAEN documentation
        // dtsp * 8ns = time difference in ns
        // In other words, do not add events with a time
        // difference between the last pulse of 1000ns
        if (dtsp*8 < 10000) {
            continue;
        }


        _pulse_file->Add(current_event);

        arma::mat wave = caen_event_to_armadillo(current_event, 1);
        _spe_estimation_pulse_buffer.row(_acq_pulses) = wave;

        _acq_pulses++;
    }

    spdlog::info("Acquired {0} pulses so far.", _acq_pulses);

    save(_pulse_file, sbc_save_func, _caen_port);
    if (_acq_pulses >= _doe.VBDData.SPEEstimationTotalPulses) {
        // We close the file here because this is the exact moment acquisition
        // stopped. In case of an error, we reopen.
        _close_sipm_file();

        spdlog::info("Finished taking data! Moving to analysis of voltage "
            "{0}", *_current_voltage);
        // This is the line of code that runs the analysis!
        _spe_analysis_out = _spe_analysis->FullAnalysis(
            _spe_estimation_pulse_buffer,
            _doe.GroupConfigs[0].TriggerThreshold, 1.0);

        spdlog::info("Finished analysis of voltage {0}", *_current_voltage);

        // Sometimes the analysis can fail and this is
        // reflected in SPEEfficiency being 0.
        // It goes to the next voltage if successful. Otherwise, it repeats.
        if (_spe_analysis_out.SPEEfficiency > 0.0) {
            _vbe_analysis->add(_spe_analysis_out.SPEParameters(1),
                _spe_analysis_out.SPEParametersErrors(1),
                _doe.LatestMeasure.Volt);

            spdlog::info("Calculated gain of {0} [arb.] with error {1} "
                "at measurement {2}V",
                _spe_analysis_out.SPEParameters(1),
                _spe_analysis_out.SPEParametersErrors(1),
                _doe.LatestMeasure.Volt);

            auto t = get_current_time_epoch() / 1000.0;
            for (arma::uword i = 0; i < _spe_analysis_out.SPEParameters.n_elem; i++) {
                _saveinfo_file->Add(SaveFileInfo(t,
                    std::to_string(_spe_analysis_out.SPEParameters(i))));

                _saveinfo_file->Add(SaveFileInfo(t,
                    std::to_string(_spe_analysis_out.SPEParametersErrors(i))));
            }

            // go to the next voltage.
            ++_current_voltage;
            _has_new_gain_measurement = true;

            // If done with all the voltages, time to move on!
            if (_current_voltage == GainVoltages.cend()) {
                spdlog::info("Finished taking gain measurements.");
                _current_state = BreakdownRoutineState::CalculateBreakdownVoltage;
            } else {
                _has_voltage_changed = true;
                // This should open the next file.
                _open_sipm_file();
            }
        } else {
            // This means the measurement failed, so we reopen the file.
            _open_sipm_file();
        }

        _acq_pulses = 0;

        async_save(_saveinfo_file, [](const SaveFileInfo& val) {
            return std::to_string(val.Time) + ","
                + val.FileName + "\n";
        });

    }

    return true;
}

bool BreakdownRoutine::_calculate_breakdown_voltage() noexcept {
    if (_vbe_analysis->size() < 3) {
        spdlog::error("There are less than three gain-voltage pairs in the "
            "buffer, what went wrong?");
    }

    double t = get_current_time_epoch() / 1000.0;
    auto values = _vbe_analysis->calculate();

    if (values.BreakdownVoltage > 0.0) {
        _vbe_analysis_out = values;
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

        async_save(_saveinfo_file, [](const SaveFileInfo& val) {
            return std::to_string(val.Time) + ","
                + val.FileName + "\n";
        });

        _current_state = BreakdownRoutineState::Finished;
        return false;
    } else {
        spdlog::error("Breakdown voltage calculation failed. Restarting.");
        _soft_reset();
        return true;
    }
}

bool BreakdownRoutine::_finished() noexcept {
    return true;
}

bool BreakdownRoutine::_soft_reset() noexcept {
    _current_state = BreakdownRoutineState::Analysis;
    _acq_pulses = 0;
    _reset_voltage();
    return true;
}

bool BreakdownRoutine::_hard_reset() noexcept {
    _close_sipm_file();
    _soft_reset();
    _current_state = BreakdownRoutineState::Init;
    return true;
}

}  // namespace SBCQueens