#pragma once

#include <inttypes.h>
#include <SPI.h>

namespace SBCQueens {
    // TeensyController registers
    // They might or not have relationship with internal registers
    struct BME280Registers_t {
        int32_t LAST_TEMP_REG      = 0;
        int32_t LAST_PRESSURE_REG  = 0;
        int32_t LAST_HUM_REG       = 0;
    };

    enum class BME280_ADDR { 
        r_ID_ADDR       = 0xD0u,
        w_RESET_ADDR    = 0x60u,
        w_CTRL_HUM_ADDR = 0x72u,
        r_STATUS_ADDR   = 0xF3u,
        w_CTRL_MEAS_ADDR  = 0x74u,
        w_CONFIG_ADDR   = 0x75u,
        r_MEAS_ADDR     = 0xF7u
    };

    /// BME280 config bits
    /// 7:5 -> tstandby duration (in ms) in normal mode
    /// 4:2 -> filter time constant coefficient (111 = 16)
    /// 1 -> not used
    /// 0 -> enables 3-wire
    enum class BME_CONFIG_VALS {
        LOWEST_T_FILTER_OFF         = 0b00000000u,
        LOWEST_T_FILTER_MIN         = 0b00000100u,
        LOWEST_T_FILTER_MAX         = 0b00011100u,
    };

    /// BME280 control bits
    /// 7:5 -> temperature oversampling (101 = x16)
    /// 4:2 -> pressure oversampling (101 = x16)
    /// 1:0 -> mode (01 or 10 = forced mode), 11 = Normal mode
    enum BME_CONTROL_VALS {
        TEMP_OFF_PRESS_OFF_SLEEP    = 0b00000000u,
        TEMP_OFF_PRESS_OFF_NORMAL   = 0b00000011u,
        TEMP_OFF_PRESS_OFF_FORCED   = 0b00000010u,
        // All ON = oversampling x 16
        // we ignore the rest
        TEMP_ON_PRESS_ON_NORMAL     = 0b11111111u,
        TEMP_ON_PRESS_ON_FORCED     = 0b11111110u,
    };

    /// BME280 humidity configuraton bits
    /// 2:0 -> humidity oversampling
    enum BME_HUM_CONTROL_VALS {
        HUM_OFF = 0,
        HUM_ON  = 0b00000111u
    };

    struct BME280_t {
        uint8_t                 CS_PIN;
        BME280Registers_t     REGISTERS;

        uint8_t                 Error           = 0u;
        BME_CONFIG_VALS         ConfValue       = BME_CONFIG_VALS::LOWEST_T_FILTER_OFF;
        BME_CONTROL_VALS        ControlValue    = BME_CONTROL_VALS::TEMP_OFF_PRESS_OFF_SLEEP;
        BME_HUM_CONTROL_VALS    HumConfValue    = BME_HUM_CONTROL_VALS::HUM_OFF;
    };

    void bme280_init(BME280_t&);
    void bme280_read_measurements(BME280_t&);
    void bme280_check_status(BME280_t&);

} // namespace SBCQueens
