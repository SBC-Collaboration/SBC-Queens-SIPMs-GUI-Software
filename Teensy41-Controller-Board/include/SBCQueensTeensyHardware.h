#pragma once

#include "Drivers/SBCQueensBME280Driver.h"
#include "Drivers/SBCQueensMAX31865Driver.h"
#include "Drivers/SBCQueensPeltierDriver.h"
#include "Drivers/SBCQueensTeensyAnalog.h"
#include "Drivers/SBCQueensRelay.h"

#include "SBCQueensPID.h"

namespace SBCQueens {

    /// PIN DEFINITIONS
 
    const uint8_t TWELVEV_PIN_ONE   = 2u;
    const uint8_t TWELVEV_PIN_TWO   = 3u;
    const uint8_t TWELVEV_PIN_THREE = 4u;

    const uint8_t RTD_ONE_CS        = 5u;
    const uint8_t RTD_TWO_CS        = 6u;
    const uint8_t BME_CS            = 7u;
    const uint8_t BOX_BME_CS        = 8u;

    const uint8_t PELTIER_EN_PIN    = 21u;

    // Analog PINS
    const uint8_t NTWO_PIN = 0u; // 15
    const uint8_t VACUUM_PIN = 1u; // 16

    ///

    extern uint16_t N2_RELEASE_MAN_REG;
    extern uint16_t N2_RELEASE_PID_REG;
    extern Relay N2_RELEASE_RELAY;
    extern Relay N2_INPUT_RELAY;

    extern BME280_t LOCAL_BME280;
    extern BME280_t BOX_BME280;

    extern PeltierDriver PELTIER_DRIVER;

    extern MAX31865_t RTD_DAC_01;
    extern MAX31865_t RTD_DAC_02;

    extern PID PELTIER_PID;
    extern PID N2_PID;

    extern AnalogReadMV VACUUM_PRESSURE_SENSOR;
    extern AnalogReadMV NTWO_PRESSURE_SENSOR;

    // Initializes the internal data structures that are going to be used
    // to control and talk to the external hardware
    void init_hardware_structs();

    // Initializes all the pins found in this file
    void init_pins();
    
} // namespace SBCQueens


