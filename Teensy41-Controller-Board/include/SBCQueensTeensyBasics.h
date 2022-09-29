#pragma once

// #define RTD_ONLY_MODE
// #define NEW_RTD_BOARD

#include <arduino_freertos.h>
#include <timers.h>
#include <semphr.h>

namespace SBCQueens {
    /// Mutex
    extern SemaphoreHandle_t SPI_mutex;
    extern SemaphoreHandle_t Wire1_mutex;
    extern SemaphoreHandle_t Wire2_mutex;
    extern SemaphoreHandle_t registers_mutex;
    /// !Mutex

    /// Timer Handles
    extern TimerHandle_t prepare_rtd_conversion_h;
    extern TimerHandle_t retrieve_rtd_conversion_h;
    extern TimerHandle_t sleep_rtd_h;

    extern TimerHandle_t update_pid_h;

    extern TimerHandle_t retrieve_bme280_measurement_h;

    extern TimerHandle_t take_pressures_meas_h;
    /// !Timer Handles

    // Initializes all the mutexes, put in setup()
    void init_teensy_controller_mutexes();

    // Takes the mutex if available
    // otherwise, it blocks
    void take_reg_mux();

    // Gives back the register mutex 
    // MUST be called after take_reg_mux()
    void give_reg_mux();

    /// Communication handlers 
    //// SPI Communcation handlers
    // Starts the SPI communication and takes the SPI comm mutex
    void start_spi(const uint8_t& cs_pin, const uint8_t& mode = SPI_MODE1);
    // Ends the SPI communication and gives the SPI comm mutex
    void end_spi(const uint8_t& pin);
    //// !SPI Communcation handlers
    /////
    //// Wire Communcation handlers
    // Starts the Wire1 communication and takes the Wire1 comm mutex
    void start_wire1(const uint8_t& addr) ;
    // Ends the Wire1 communication and gives the Wire1 comm mutex
    uint8_t end_wire1();
    // Starts the Wire2 communication and takes the Wire2 comm mutex
    void start_wire2(const uint8_t& addr) ;
    // Ends the Wire1 communication and gives the Wire1 comm mutex
    uint8_t end_wire2() ;
    //// !Wire Communcation handlers
    /// !Communication handlers 

    // Turns any value, T, into an uint8_t. 
    // Specifically made for enums in mind.
    template<typename T>
    constexpr uint8_t cast(const T& t) {
        return static_cast<uint8_t>(t);
    }

    // Turns any value, T, into an uint16_t. 
    // Specifically made for enums in mind.
    template<typename T>
    constexpr uint16_t lcast(const T& t) {
        return static_cast<uint16_t>(t);
    }
    
} // namespace SBCQueens