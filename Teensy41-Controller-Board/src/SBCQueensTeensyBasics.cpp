#include "SBCQueensTeensyBasics.h"

namespace SBCQueens {

    TimerHandle_t prepare_rtd_conversion_h      = NULL;
    TimerHandle_t retrieve_rtd_conversion_h     = NULL;
    TimerHandle_t sleep_rtd_h                   = NULL;

    TimerHandle_t update_pid_h = NULL;

    TimerHandle_t retrieve_bme280_measurement_h = NULL;

    TimerHandle_t take_pressures_meas_h         = NULL;

    SemaphoreHandle_t  SPI_mutex = NULL;
    SemaphoreHandle_t  SPI1_mutex = NULL;
    SemaphoreHandle_t  Wire1_mutex  = NULL;
    SemaphoreHandle_t  Wire2_mutex = NULL;
    SemaphoreHandle_t  registers_mutex =  NULL;

    void init_teensy_controller_mutexes() {
        // Init space for all the mutexes (? is this how you pluralize it)
        SPI_mutex = xSemaphoreCreateBinary();
        SPI1_mutex = xSemaphoreCreateBinary();
        Wire1_mutex = xSemaphoreCreateBinary();
        Wire2_mutex = xSemaphoreCreateBinary();
        registers_mutex = xSemaphoreCreateBinary();
    }

    void take_reg_mux() {
        while (xSemaphoreTake(registers_mutex, 0));
    }

    void give_reg_mux() {
        xSemaphoreGive(registers_mutex);
    }

    /// Communication handlers 
    //// SPI Communcation handlers
    // An SPI communication starts by asserting the CS pin LOW
    void start_spi(const uint8_t& cs_pin, const uint8_t& mode) {
        // Wait until the mutex is available and return it
        while (xSemaphoreTake(SPI_mutex, 0));
        SPI.beginTransaction(SPISettings(1000000, arduino::MSBFIRST, mode));
        digitalWrite(cs_pin, arduino::LOW);
    }

    // The SPI communcation ends by asserting CS pin HIGH
    void end_spi(const uint8_t& pin) {
        digitalWrite(pin, arduino::HIGH);
        SPI.endTransaction();
        // Returns the mutex
        xSemaphoreGive(SPI_mutex);
    }

    //// SPI Communcation handlers
    // An SPI communication starts by asserting the CS pin LOW
    void start_spi1(const uint8_t& cs_pin, const uint8_t& mode) {
        // Wait until the mutex is available and return it
        while (xSemaphoreTake(SPI1_mutex, 0));
        SPI1.beginTransaction(SPISettings(1000000, arduino::MSBFIRST, mode));
        digitalWrite(cs_pin, arduino::LOW);
    }

    // The SPI communcation ends by asserting CS pin HIGH
    void end_spi1(const uint8_t& pin) {
        digitalWrite(pin, arduino::HIGH);
        SPI1.endTransaction();
        // Returns the mutex
        xSemaphoreGive(SPI1_mutex);
    }
    //// !SPI Communcation handlers
    /////
    //// Wire Communcation handlers
    void start_wire1(const uint8_t& addr) {
        while (xSemaphoreTake(Wire1_mutex, 0));
        Wire.beginTransmission(addr);
    }

    uint8_t end_wire1() {
        uint8_t err = Wire.endTransmission();
        xSemaphoreGive(Wire1_mutex);

        return err;
    }

    void start_wire2(const uint8_t& addr) {
        while (xSemaphoreTake(Wire2_mutex, 0));
        Wire2.beginTransmission(addr);
    }

    uint8_t end_wire2() {
        uint8_t err = Wire2.endTransmission();
        xSemaphoreGive(Wire2_mutex);

        return err;
    }
    //// !Wire Communcation handlers
    /// !Communication handlers 
} // namespace SBCQueens
