#include "Drivers/SBCQueensPeltierDriver.h"

#include <Wire.h>

#include "SBCQueensTeensyBasics.h"

namespace SBCQueens {

    void peltierdriver_init(PeltierDriver& controller) {

        pinMode(controller.EN_PIN, arduino::OUTPUT);
        digitalWrite(controller.EN_PIN, arduino::LOW);
        
        uint8_t temp_buffer[] = {0, 0, 0};
        // For the ADC first send the reg addr
        temp_buffer[0] = CURRENT_ADC_REG_ADDR::CONFIG_ADDR;
        // Then its values
        temp_buffer[1] = controller.CurrentADCconfig >> 8;
        temp_buffer[2] = controller.CurrentADCconfig;
        
        start_wire1(CURRENT_ADC_I2C_ADDR);
            controller.WireStream->write(temp_buffer, 3);
        end_wire1();

        start_wire1(CURRENT_ADC_I2C_ADDR);
            controller.WireStream->write(cast(CURRENT_ADC_REG_ADDR::CONVERSION_ADDR));
        end_wire1();

        /// index 1 and 2 just initializes dac to 0V
        // For the DAC, send the config values then the DAC output values (under this config)
        uint8_t dac_temp_buffer[] = {controller.OutputDACconfig, 0x00, 0x00};
    #ifdef WRITE_TO_ERPROM
        // If this flag is enabled it changed the 3 MSB to 011
        // This permanently changed the eeeprom
        dac_temp_buffer[0] = dac_temp_buffer[0] & 0xE0;
        dac_temp_buffer[0] |= 0x60;
    #endif
        start_wire1(OUTPUT_DAC_REG_I2C_ADDR);
            controller.WireStream->write(dac_temp_buffer, 3);
        end_wire1();
    }

    void peltierdriver_enable(PeltierDriver& controller) {

        digitalWrite(controller.EN_PIN, arduino::LOW);

    }

    void peltierdriver_disable(PeltierDriver& controller) {

        digitalWrite(controller.EN_PIN, arduino::HIGH);

    }

    void peltierdriver_start_current_meas(PeltierDriver& controller) {
        
        uint8_t temp_buffer[] = {0, 0, 0};
        temp_buffer[0] = CURRENT_ADC_REG_ADDR::CONFIG_ADDR;
        temp_buffer[1] = controller.CurrentADCconfig >> 8;
        // temp_buffer[1] |= START_CONVERSION_MASK; // Enables the start a single conversion
        temp_buffer[2] = controller.CurrentADCconfig;
        
        start_wire1(CURRENT_ADC_I2C_ADDR);
            controller.WireStream->write(temp_buffer, 3);
        end_wire1();
        
    }

    void peltierdriver_retrieve_current_meas(PeltierDriver& controller) {

        uint16_t latest_current_reg = 0;
        uint8_t tmp_error = 0;

        start_wire1(CURRENT_ADC_I2C_ADDR);
            controller.WireStream->write(cast(CURRENT_ADC_REG_ADDR::CONVERSION_ADDR));
            // If false, it does not release the resources
            tmp_error = Wire.endTransmission(false);
            // tmp_error |= end_wire1();

            if(tmp_error == 0){
                uint8_t n = Wire.requestFrom(CURRENT_ADC_I2C_ADDR, static_cast<uint8_t>(2));
                
                if(n != 0) {
                    if(Wire.available()) {
                        latest_current_reg = Wire.read() << 4;
                        latest_current_reg |= (Wire.read() >> 4) & 0x0F;
                    }
                }
            }

        end_wire1();

        // We are done with all the communication, now we just need to change the
        // register to a current measurement and save it to the register
        float latest_current = __register_to_current(latest_current_reg);
        take_reg_mux();
            controller.REGISTERS.LAST_CURRENT_REG = latest_current;
            controller.REGISTERS.ERROR = tmp_error;
        give_reg_mux();
    }

    void peltierdriver_set_dac(PeltierDriver& controller) {
        // Mask to 12 bits
        uint16_t dac_reg = controller.REGISTERS.LAST_DAC_REG & 0x0FFF;
        uint8_t tmp_buffer[] = {0, 0};
        tmp_buffer[0] = dac_reg >> 8;
        tmp_buffer[1] = dac_reg; 

        // if(controller.REGISTERS.LAST_CURRENT_REG < controller.MAX_CURRENT) {
            start_wire1(OUTPUT_DAC_REG_I2C_ADDR);
                controller.WireStream->write(tmp_buffer, 2);
            end_wire1();
        // }

    }

    float __register_to_current(const uint16_t& reg) {

        return c_dac_volt_to_shunt_volt*c_shunt_volt_to_current*static_cast<float>(reg);

    }

    uint16_t __voltage_to_register(float val) {
        return 0;
    }
} // namespace SBCQueens