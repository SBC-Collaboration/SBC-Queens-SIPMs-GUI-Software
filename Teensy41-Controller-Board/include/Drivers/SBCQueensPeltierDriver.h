#pragma once

#include <inttypes.h>
#include <Stream.h>

namespace SBCQueens {
    /*

        A peltier driver consists of two I2C devices:

        1.- TLA2022 12-bit ADC 
            It reads a current sense op-amp.
        2.- MCP4706 DAC
            Sets LED driver voltage reference which then in controls
            the current output (0 - 6.2)A

    */

    // TeensyController registers
    // They might or not have relationship with internal registers
    struct PeltierDriverRegisters
    {
        float LAST_CURRENT_REG      	= 0.0;
        uint16_t LAST_DAC_REG           = 0;
        uint8_t ERROR                   = 0;
    };

    /// Current sense ADC section
    const uint8_t CURRENT_ADC_I2C_ADDR = 0b1001000;
    enum CURRENT_ADC_REG_ADDR {
        CONVERSION_ADDR = 0x00u,
        CONFIG_ADDR     = 0x01u
    };

    /// Peltier driver current sense ADC configuration bits
    // 15 MSB,  OS.                 1 -> start a single conversion
    // 14:12,   Input muliplexer.   000 -> AINp = AIN0; AINn -> AIN1
    // 11:9,    PGA.                111 -> FSR = 0.256V; 010 -> FSR = 2.048 (default)
    // 8,       Mode.               0 -> Continuous-conversion; 1-> Single-shot conversion
    // 7:5,     Sample rate.        000 -> 128 SPS; 100 -> 1600 SPS (default)
    // 4:0 LSB, Reserved.           ALWAYS should be 03h or 00011
    const uint8_t START_CONVERSION_MASK = 0x80;
    enum CURRENT_ADC_CONFIG_VALS {
        ADC_PID_OPTIMIZED_CONFIG    = 0x0E03lu,
        ADC_DEFAULT_CONFIG          = 0x0483lu
    };
    /// !Current sense ADC section
    ////
    /// Current DAC section
    const uint8_t OUTPUT_DAC_REG_I2C_ADDR = 0b1100000;
    const uint8_t FAST_DAC_VAL_REG = 0x00;
    /// Peltier driver current controlling DAC configuration bits
    // 7:5 MSB, Config.             011 -> Write to voltatile memory;
    // 4:3,     Vref.               11 -> Buffered Vref pin; 00 -> Unbuffered Vdd
    // 2:1,     Power down.         00 -> Not powered down; 01  -> powered down Vout to 1k Ohm.
    // 1,       Gain.               1 -> 2x gain, 0; 1x gain
    enum OUTPUT_DAC_CONFIG_VALS {
        DAC_PID_OPTIMIZED_CONFIG                = 0x58u,
        DAC_DEFAULT_CONFIG                      = 0x40u
    };
    /// !Current DAC section

    struct PeltierDriver
    {
        Stream* WireStream;
        uint8_t EN_PIN;

        float MAX_CURRENT = 5.0; // amps
        
        PeltierDriverRegisters REGISTERS;
        bool RegistersChanged                     = false;

        CURRENT_ADC_CONFIG_VALS CurrentADCconfig  = CURRENT_ADC_CONFIG_VALS::ADC_DEFAULT_CONFIG;
        OUTPUT_DAC_CONFIG_VALS  OutputDACconfig   = OUTPUT_DAC_CONFIG_VALS::DAC_DEFAULT_CONFIG;
    };

    // Initializes communication with the Peltier ADC and DAC, restarts them, 
    // and sends the initial configuration bits.
    void peltierdriver_init(PeltierDriver&);

    // Enables the PID by turning ON the relay.
    void peltierdriver_enable(PeltierDriver&);

    // Disables the PID by turning OFF the relay.
    void peltierdriver_disable(PeltierDriver&);

    // Initializes a current measurement by sending the appropiate registers.
    void peltierdriver_start_current_meas(PeltierDriver&);

    // Retrieves the current measurements initialized by peltierdriver_start_current_meas
    // by sending the appropiate registers, and saves the resulting values into
    // PeltierDriver*->REGISTERS.LAST_DAC_REG
    void peltierdriver_retrieve_current_meas(PeltierDriver&);

    // Sets the DAC output voltage by sending the appropiate register values.
    // If the current exceeds the set max limit, the value won't change.
    void peltierdriver_set_dac(PeltierDriver&);

    /// Math specific functions
    const float c_dac_volt_to_shunt_volt = 0.256 / 2048.0;
    const float c_shunt_volt_to_current = 24.20721375;
    float __register_to_current(const uint16_t& reg);
    uint16_t __voltage_to_register(const float& val);
    
} // namespace SBCQueens