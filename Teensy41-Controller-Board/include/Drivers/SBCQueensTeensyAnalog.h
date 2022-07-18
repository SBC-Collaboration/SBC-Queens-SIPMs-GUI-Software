#pragma once

#include <inttypes.h>

namespace SBCQueens {

    // analogReadResolution
    struct AnalogRead_t {

        uint8_t PIN;

        uint16_t LATEST_VALUE;

    };

    void AnalogRead_init(AnalogRead_t&);
    void AnalogRead_acquire(AnalogRead_t&);

/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 200 Hz

* 0 Hz - 40 Hz
  gain = 1
  desired ripple = 1 dB
  actual ripple = 0.7139661994197533 dB

* 50 Hz - 100 Hz
  gain = 0
  desired attenuation = -40 dB
  actual attenuation = -40.45063480499388 dB

*/

#define FILTER_TAP_NUM 33

static double filter_taps[FILTER_TAP_NUM] = {
  0.004589972864584906,
  0.017081797717549866,
  0.008482862272062313,
  -0.009028258250168862,
  -0.013894462082291713,
  0.007383794907705269,
  0.02213164989047186,
  -0.0010160148859201287,
  -0.03177359501338608,
  -0.012213811683435577,
  0.0412775793059865,
  0.03686474619732638,
  -0.04931502207050167,
  -0.08869657166183911,
  0.05468201162918776,
  0.31226693992501803,
  0.4434288348694254,
  0.31226693992501803,
  0.05468201162918776,
  -0.08869657166183911,
  -0.04931502207050167,
  0.03686474619732638,
  0.0412775793059865,
  -0.012213811683435577,
  -0.03177359501338608,
  -0.0010160148859201287,
  0.02213164989047186,
  0.007383794907705269,
  -0.013894462082291713,
  -0.009028258250168862,
  0.008482862272062313,
  0.017081797717549866,
  0.004589972864584906
};




    // Intended to be sampled at 1KHz
    struct AnalogReadFIR_t {

        uint8_t PIN;

        // This will count up for every sample taken in AnalogReadFIR_acquire
        uint8_t SAMPLES_TAKEN = 0;
        float LATEST_VALUES[FILTER_TAP_NUM];

        // Every FILTER_TAP_NUM*1KHz = 59ms there will be a new measurement
        float LATEST_VALUE = 0;

    };

    void AnalogReadFIR_init(AnalogReadFIR_t&);
    void AnalogReadFIR_acquire(AnalogReadFIR_t&);


    // Moving average
    struct AnalogReadMV {

        const uint8_t MAX_SAMPLES = 32;
        uint8_t PIN;

        // This will count up for every sample taken in AnalogReadFIR_acquire
        uint8_t SAMPLES_TAKEN = 0;
        float LATEST_VALUES[32];

        // Every FILTER_TAP_NUM*1KHz = 59ms there will be a new measurement
        float LATEST_VALUE = 0;

    };

    void AnalogReadMV_init(AnalogReadMV&);
    void AnalogReadMV_acquire(AnalogReadMV&);


} // namesapce SBCQueens