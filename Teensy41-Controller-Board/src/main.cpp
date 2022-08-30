#include <string.h>
#include <stdlib.h>

// Arduino/Teensy includes
#include <SPI.h>
#include <Wire.h>

// FreeRTOS includes
#include <arduino_freertos.h>
#include <timers.h>
#include <semphr.h>

// Other
#include <CRC32.h>

// My libraries includes
#define RTD_ONLY_MODE
#define NEW_RTD_BOARD

#include "SBCQueensTeensyBasics.h"
#include "SBCQueensTeensyCommunications.h"
#include "SBCQueensTeensyHardware.h"
#include "SBCQueensTiming.h"

#include "SBCQueensPID.h"



// Tasks:
// Timer - PID1
// Timer - PID2
// Timer - PID3 (all timers 100ms)
// Timer - Room Temperature/Pressure/Humidity sensor1 (1-2 Hz)
// Task - Serial communication - Unblocks on serial input
// Task - Main, error & Status LED Handler - Unblocks on error,
//      on saving data or any other minimal task

// Critical resources:
// All SPI, and the main serial resource
// All registers
// Everything else is game

// (uint32_t)pvTimerGetTimerID(xTimer) returns the
// timer id

// All SPI are critical (!)
// but I2C ARE NOT critical!

// It turns on the current sources for the RTDs.
// It is the start of the RTD measurement chain.
void prepare_rtd_conversion_cb(TimerHandle_t xTimer) {

#ifdef NEW_RTD_BOARD
    for(uint8_t i = 0; i < SBCQueens::NUM_RTD_BOARDS; i++) {
        SBCQueens::RTDBoard_prepare_meas(SBCQueens::RTD_BOARDS[i]);
    }
#else
    // The temperature sensor needs an extra step to take
    // a measurement. We require to enable Vbias and wait 10 ms
    SBCQueens::max31865_prepare_measurement(SBCQueens::RTD_DAC_01);
    SBCQueens::max31865_prepare_measurement(SBCQueens::RTD_DAC_02);
#endif

    // Trigger the new timer
    xTimerReset(SBCQueens::retrieve_rtd_conversion_h, 0);

}

// Called 10ms after prepare_pid_conversion_callback
// Starts a RTD and current measurement
// then triggers the callback to retrieve the data in 52 ms
void retrieve_rtd_conversion_cb(TimerHandle_t xTimer) {
    
    /// Temperature sensor ////
#ifdef NEW_RTD_BOARD
    for(uint8_t i = 0; i < SBCQueens::NUM_RTD_BOARDS; i++) {
        SBCQueens::RTDboard_take_meas(SBCQueens::RTD_BOARDS[i]);
    }
#else
    SBCQueens::max31865_start_measurement(SBCQueens::RTD_DAC_01);
    SBCQueens::max31865_start_measurement(SBCQueens::RTD_DAC_02);
#endif

    /// Current sensor ///
#ifndef RTD_ONLY_MODE
    SBCQueens::peltierdriver_start_current_meas(SBCQueens::PELTIER_DRIVER);
#endif
    // Now we wait for acquisiton time ms
    xTimerReset(SBCQueens::sleep_rtd_h, 0);

}

// Retrieves the measuremnts from the RTDs and current.
// It does not trigger another callback
void sleep_rtd_cb(TimerHandle_t xTimer) {

    /// Temperature sensor ///

#ifdef NEW_RTD_BOARD
    for(uint8_t i = 0; i < SBCQueens::NUM_RTD_BOARDS; i++) {
        SBCQueens::RTDboard_standby(SBCQueens::RTD_BOARDS[i]);
    }

    // for(uint8_t i = 0; i < SBCQueens::NUM_RTD_BOARDS; i++) {
    //     SBCQueens::RTDboard_translate_meas(SBCQueens::RTD_BOARDS[i]);
    // }
#else 
    SBCQueens::max31865_retrieve_measurement(SBCQueens::RTD_DAC_01);
    SBCQueens::max31865_retrieve_measurement(SBCQueens::RTD_DAC_02);
#endif

    /// Current sensor ///
#ifndef RTD_ONLY_MODE
    SBCQueens::peltierdriver_retrieve_current_meas(SBCQueens::PELTIER_DRIVER);
#endif
}

// Reads the latest BME280 measurements every 114 ms
void retrieve_bme280_measurement_cb(TimerHandle_t xTimer) {
    
#ifndef RTD_ONLY_MODE
    SBCQueens::bme280_read_measurements(SBCQueens::LOCAL_BME280);
#endif

}

// Reads from the analog ports the latest pressures. This happens every 5ms
// However, a measurement is only ready 5ms*32 = 160ms because of the moving
// average
void take_pressures_meas_cb(TimerHandle_t xTimer) {

#ifndef RTD_ONLY_MODE
    SBCQueens::AnalogReadMV_acquire(SBCQueens::VACUUM_PRESSURE_SENSOR);
#endif

}

void update_pid_cb(TimerHandle_t xTimer) {
    #ifndef RTD_ONLY_MODE
        SBCQueens::TCPID_update(SBCQueens::PELTIER_PID);
        SBCQueens::peltierdriver_set_dac(SBCQueens::PELTIER_DRIVER);
    #endif
}

// Listens to the serial, parses the texts and executes if valid.
// Only task with access to the serial.
// See SBCQueensTeensyCOmmunications.h/cpp for more information on the commands
void serial_task(void* parameters) {

    float command_value  = 0.0;
    bool has_value = false;
    uint32_t calculated_checksum = 0;
    char* tmp_str_ptr = NULL;
    // Floats do not make sense if they go beyond 6 precision, so 6 + 1 + 1 = 8, 8 + 10 = 18
    // obligatory ; ' ' \n, 18 + 3 = 21
    // we are left with 14 characters left for command
    char serial_buffer[35];

    while(1) {
        if(Serial.available() > 0) {
            // I expect a command to always come in the form:
            // COMMAND VALUE;
            // VALUE being optional for some commands
            Serial.readBytesUntil('\n', serial_buffer, 35);
            
            tmp_str_ptr = strchr(serial_buffer, ';');
            
            if(tmp_str_ptr == NULL) {
                Serial.print(SBCQueens::c_NACK_str);
                continue;
            } else {
                
                // After the last operation, we can then look for the 
                // value if there is some
                tmp_str_ptr = strchr(serial_buffer, ' ');
                
                // If there is a separator that means
                // the command is a two parter
                has_value = tmp_str_ptr != NULL;
                if (has_value) {
                    tmp_str_ptr++;
                    command_value = atof(tmp_str_ptr);
                    tmp_str_ptr = strtok(serial_buffer, " ");
                } else {
                    tmp_str_ptr = strtok(serial_buffer, ";");
                }
                
                // finally but not least, get the command
                // strtok MODIFIES the buffer so from here
                // assume serial_buffer is different from when
                // we started

                // Do checksum check here
                calculated_checksum = SBCQueens::c_crc32_checksum(tmp_str_ptr);
                // Only run the commands if the checksum passes

                SBCQueens::take_reg_mux();
                // First send any ACK str then it sends the rest of the data
                Serial.print(SBCQueens::c_ACK_str);

                if(!SBCQueens::check_and_run(calculated_checksum, command_value)) {
                    Serial.print(SBCQueens::c_INVALID_CMD_str);
                }

                SBCQueens::give_reg_mux();
            }
        }
    }

}

void local_devices_initialization() {

    SBCQueens::bme280_init(SBCQueens::LOCAL_BME280);

    SBCQueens::peltierdriver_init(SBCQueens::PELTIER_DRIVER);

}

FLASHMEM __attribute__((noinline)) void setup()
{
    /// Communications Initialization
    // The Serial.begin(...) is not required for Teensy 4.1
    // It always communicates at USB speeds (~400 MB/s)
    // Serial.begin(2500000);
    SPI.begin();
    Wire.begin();
    Wire1.begin();
    // /// !Communications Initialization

    // Hardware Initializations
    SBCQueens::init_teensy_controller_mutexes();
    SBCQueens::init_pins();
    SBCQueens::init_hardware_structs();
    local_devices_initialization();

    SBCQueens::TCPID_init(SBCQueens::PELTIER_PID);
    // /// !Hardware Initializations

    // // Tasks section
    xTaskCreate(
        serial_task,                // Callback function
        "Serial",                   // Name
        1024,                       // Stack size
        NULL,                       // ???
        1,                          // Priority
        NULL                        // ???
    );

    /// Timer initializations

    SBCQueens::prepare_rtd_conversion_h = xTimerCreate(
        "prepare_pid_covnersion_1",                 // Name
        pdMS_TO_TICKS(SBCQueens::RTDSamplingTime),  // Timer period in ticks
        pdTRUE,                                     // true for auto reload, false is manual reload
        (void*)0,                                   // Timer ID
        prepare_rtd_conversion_cb                   // Callback function
    );

    if (SBCQueens::prepare_rtd_conversion_h != NULL) {
        xTimerStart(SBCQueens::prepare_rtd_conversion_h, portMAX_DELAY);
    }

    SBCQueens::retrieve_rtd_conversion_h = xTimerCreate(
        "init_pid_conversion_1",
        pdMS_TO_TICKS(SBCQueens::c_RTDPrepareTime), 
        pdFALSE, 
        (void*)2,
        retrieve_rtd_conversion_cb
    );

    SBCQueens::sleep_rtd_h = xTimerCreate(
        "retrieve_pid_conversion_1", 
        pdMS_TO_TICKS(SBCQueens::c_RTDAcquisitionTime), 
        pdFALSE, 
        (void*)3,
        sleep_rtd_cb
    );

    SBCQueens::retrieve_bme280_measurement_h = xTimerCreate(
        "retrieve_bme280_conversion", 
        pdMS_TO_TICKS(SBCQueens::BMESamplingTime), 
        pdTRUE, 
        (void*)4,
        retrieve_bme280_measurement_cb
    );

    if(SBCQueens::retrieve_bme280_measurement_h != NULL) {
        xTimerStart(SBCQueens::retrieve_bme280_measurement_h, portMAX_DELAY);
    }

    SBCQueens::take_pressures_meas_h = xTimerCreate(
        "take_pressures_meas", 
        pdMS_TO_TICKS(SBCQueens::PressureTransSamplingTime), 
        pdTRUE, 
        (void*)5,
        take_pressures_meas_cb
    );

    if(SBCQueens::take_pressures_meas_h != NULL) {
        xTimerStart(SBCQueens::take_pressures_meas_h, portMAX_DELAY);
    }

    SBCQueens::update_pid_h = xTimerCreate(
        "take_pressures_meas", 
        pdMS_TO_TICKS(SBCQueens::PIDUpdateTime), 
        pdTRUE, 
        (void*)6,
        update_pid_cb
    );

    if(SBCQueens::update_pid_h != NULL) {
        xTimerStart(SBCQueens::update_pid_h, portMAX_DELAY);
    }

    /// !Timer initialization
    vTaskStartScheduler();

    // Deletes setup and loop tasks
    vTaskDelete(NULL);

    
}

void loop() {}
