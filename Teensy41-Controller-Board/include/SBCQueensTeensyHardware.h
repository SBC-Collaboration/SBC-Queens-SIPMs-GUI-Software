#pragma once

#include "Drivers/SBCQueensBME280Driver.h"
#include "Drivers/SBCQueensMAX31865Driver.h"
#include "Drivers/SBCQueensPeltierDriver.h"
#include "Drivers/SBCQueensTeensyAnalog.h"
#include "Drivers/SBCQueensRTDBoard.h"
#include "Drivers/SBCQueensRelay.h"

#include "SBCQueensPID.h"

// #define SIPM_SETUP

namespace SBCQueens {

    /// PIN DEFINITIONS
 
#ifndef RTD_ONLY_MODE
    const uint8_t TWELVEV_PIN_ONE   = 30u;
    const uint8_t TWELVEV_PIN_TWO   = 31u;
    const uint8_t TWELVEV_PIN_THREE = 32u;

#ifndef NEW_RTD_BOARD
    const uint8_t RTD_ONE_CS        = 24u;
    const uint8_t RTD_TWO_CS        = 25u;
    const uint8_t RTD_THREE_CS      = 28u;
#endif

    const uint8_t BME_CS            = 0u;

    const uint8_t PELTIER_EN_PIN    = 17u;

    // Analog PINS
    const uint8_t NTWO_PIN = 0u; // 15
    const uint8_t VACUUM_PIN = 9u; //23

    ///
    extern BME280_t LOCAL_BME280;

    extern PeltierDriver PELTIER_DRIVER;
    extern PID PELTIER_PID;
    extern AnalogReadMV VACUUM_PRESSURE_SENSOR;
    
#endif

#ifdef NEW_RTD_BOARD

#ifdef SIPM_SETUP
    const uint8_t NUM_RTD_BOARDS = 1;
    const uint8_t NUM_RTD_PER_BOARD = 3;
#else
    const uint8_t NUM_RTD_BOARDS = 2;
    const uint8_t NUM_RTD_PER_BOARD = 3;
#endif
    extern RTDBoard<NUM_RTD_PER_BOARD> RTD_BOARDS[NUM_RTD_BOARDS];
#else
    const uint8_t NUM_RTD_BOARDS = 3;
    const uint8_t NUM_RTD_PER_BOARD = 1;
    extern MAX31865_t RTD_BOARDS[NUM_RTD_BOARDS];
#endif

    // Initializes the internal data structures that are going to be used
    // to control and talk to the external hardware
    void init_hardware_structs();

    // Initializes all the pins found in this file
    void init_pins();
    
} // namespace SBCQueens


