#include "sbcqueens-gui/sipm_helpers/AcquisitionRoutine.hpp"

namespace SBCQueens {

bool AcquisitionRoutine::update() noexcept {
	if (_has_finished) {
		return false;
	}

    _has_new_events = _process_events();
    _has_voltage_changed = false;
    _has_new_gain_measurement = false;

    if (not _has_new_events) {
    	return false;
    }

    // Do not take data if temp or voltage is not stable.
    // Data taking is still taking in the background to make sure the buffer
    // is clear once these are stabilized
    if (not _doe.isVoltageStabilized || not _doe.isTemperatureStabilized) {
        return true;
    }

    // If the pulse file is not open, open it with the current setup.
    if (not _pulse_file) {
    	_open_sipm_file();
    }

    // k = 1, we always throw the first event
    for (uint32_t k = 1; k < _latest_num_events; k++) {

        // If we already have enough events, do not grab more
        if (_acq_pulses >= _doe.VBDData.DataPulses) {
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
        _acq_pulses++;
    }

    save(_pulse_file, sbc_save_func, _caen_port);

    if (_acq_pulses >= _doe.VBDData.DataPulses) {
    	// Reset
    	_acq_pulses = 0;
        // Go to the next voltage.
        ++_current_overvoltage;

		_close_sipm_file();

        // If done with all the voltages, time to move on!
        if (_current_overvoltage != OverVoltages.cend()) {
            _has_voltage_changed = true;
        } else {
        	_has_finished = true;
        }
    }
}

void AcquisitionRoutine::reset() noexcept {
	_close_sipm_file();
	_has_finished = false;
	_reset_voltage();
}

}  // namespace SBCQueens