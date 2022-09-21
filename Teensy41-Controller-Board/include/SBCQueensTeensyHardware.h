#pragma once

#include "Drivers/SBCQueensBME280Driver.h"
#include "Drivers/SBCQueensMAX31865Driver.h"
#include "Drivers/SBCQueensPeltierDriver.h"
#include "Drivers/SBCQueensTeensyAnalog.h"
#include "Drivers/SBCQueensRTDBoard.h"
#include "Drivers/SBCQueensRelay.h"

#include "SBCQueensPID.h"



namespace SBCQueens {

    /// PIN DEFINITIONS
 
#ifndef RTD_ONLY_MODE
    const uint8_t TWELVEV_PIN_ONE   = 2u;
    const uint8_t TWELVEV_PIN_TWO   = 3u;
    const uint8_t TWELVEV_PIN_THREE = 4u;

#ifndef NEW_RTD_BOARD
    const uint8_t RTD_ONE_CS        = 5u;
    const uint8_t RTD_TWO_CS        = 6u;
#endif

    const uint8_t BME_CS            = 7u;
    const uint8_t BOX_BME_CS        = 8u;

    const uint8_t PELTIER_EN_PIN    = 21u;

    // Analog PINS
    const uint8_t NTWO_PIN = 0u; // 15
    const uint8_t VACUUM_PIN = 1u; // 16

    ///

    extern BME280_t LOCAL_BME280;

    extern PeltierDriver PELTIER_DRIVER;
    extern PID PELTIER_PID;
    extern AnalogReadMV VACUUM_PRESSURE_SENSOR;
    
#endif

#ifdef NEW_RTD_BOARD
    const uint8_t NUM_RTD_BOARDS = 2;
    const uint8_t NUM_RTD_PER_BOARD = 3;
    extern RTDBoard<NUM_RTD_PER_BOARD> RTD_BOARDS[NUM_RTD_BOARDS];
#else
    const uint8_t NUM_RTD_BOARDS = 4;
    const uint8_t NUM_RTD_PER_BOARD = 1;
    extern MAX31865_t RTD_BOARDS[NUM_RTD_BOARDS];
#endif

    // Initializes the internal data structures that are going to be used
    // to control and talk to the external hardware
    void init_hardware_structs();

    // Initializes all the pins found in this file
    void init_pins();
    
} // namespace SBCQueens


