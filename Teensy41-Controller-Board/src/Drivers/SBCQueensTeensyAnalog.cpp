#include "Drivers/SBCQueensTeensyAnalog.h"

#include <arduino_freertos.h>

#include "SBCQueensTeensyBasics.h"

namespace SBCQueens {

    void AnalogRead_init(AnalogRead_t& mod) {

        pinMode(mod.PIN, arduino::INPUT_DISABLE);
        analogReadResolution(12);
        mod.LATEST_VALUE = 0;
        
    }

    void AnalogRead_acquire(AnalogRead_t& mod) {
        take_reg_mux();
            mod.LATEST_VALUE = analogRead(mod.PIN);
        give_reg_mux();
    }

    void AnalogReadFIR_init(AnalogReadFIR_t& mod) {

        pinMode(mod.PIN, arduino::INPUT_DISABLE);
        analogReadResolution(12);

        mod.LATEST_VALUE = 0;

        for(int8_t i = 0; i < FILTER_TAP_NUM; i++) {
            mod.LATEST_VALUES[i] = 0;
        }

    }

    void AnalogReadFIR_acquire(AnalogReadFIR_t& mod) {
        mod.LATEST_VALUES[mod.SAMPLES_TAKEN] = analogRead(mod.PIN);

        mod.SAMPLES_TAKEN++;
        if(mod.SAMPLES_TAKEN >= FILTER_TAP_NUM) {
            float total = 0;
            for(int8_t i = 0; i < FILTER_TAP_NUM; i++) {
                total += mod.LATEST_VALUES[i]*filter_taps[i];
                mod.LATEST_VALUES[i] = 0;
            }

            take_reg_mux();
                mod.LATEST_VALUE = total;
                mod.SAMPLES_TAKEN = 0;
            give_reg_mux();
        }

    }

    void AnalogReadMV_init(AnalogReadMV& mod) {

        pinMode(mod.PIN, arduino::INPUT_DISABLE);
        analogReadResolution(12);
        mod.LATEST_VALUE = 0;

        for(int8_t i = 0; i < FILTER_TAP_NUM; i++) {
            mod.LATEST_VALUES[i] = 0;
        }
    }

    void AnalogReadMV_acquire(AnalogReadMV& mod) {

        mod.LATEST_VALUES[mod.SAMPLES_TAKEN] = analogRead(mod.PIN);

        mod.SAMPLES_TAKEN++;
        if(mod.SAMPLES_TAKEN >= mod.MAX_SAMPLES) {
            float total = 0;
            for(int8_t i = 0; i < mod.MAX_SAMPLES; i++) {
                total += (1.0/mod.MAX_SAMPLES)*mod.LATEST_VALUES[i];
                mod.LATEST_VALUES[i] = 0;
            }

            take_reg_mux();
                mod.LATEST_VALUE = total;
                mod.SAMPLES_TAKEN = 0;
            give_reg_mux();
        }

    }

} //namespace SBCQueens