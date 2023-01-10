#include "Drivers/SBCQueensBME280Driver.h"

#include "SBCQueensTeensyBasics.h"

namespace SBCQueens {

    void bme280_init(BME280_t& controller) {
        
        // First, soft reset
        start_spi1(controller.CS_PIN, SPI_MODE0);
            SPI.transfer( cast ( BME280_ADDR::w_RESET_ADDR) );
            SPI.transfer(0xB6);
        end_spi1(controller.CS_PIN);
        
        // wait a bit
        delay(10);

        // Then, set in sleep mode, everything off
        start_spi1(controller.CS_PIN, SPI_MODE0);
            SPI.transfer( cast ( BME280_ADDR::w_CTRL_MEAS_ADDR ) );
            SPI.transfer( cast ( BME_CONTROL_VALS::TEMP_OFF_PRESS_OFF_SLEEP) );
        end_spi1(controller.CS_PIN);

        // Then, we modify control measure for temp and pressure
        start_spi1(controller.CS_PIN, SPI_MODE0);
            SPI.transfer(cast(BME280_ADDR::w_CONFIG_ADDR));
            SPI.transfer(cast(controller.ConfValue));
        end_spi1(controller.CS_PIN);

        // Next, set humidify configuration measurements
        start_spi1(controller.CS_PIN, SPI_MODE0);
            SPI.transfer(cast(BME280_ADDR::w_CTRL_HUM_ADDR));
            SPI.transfer(cast(controller.HumConfValue));
        end_spi1(controller.CS_PIN);

        // Finally, the control bits for pressure and temp
        start_spi1(controller.CS_PIN, SPI_MODE0);
            SPI.transfer(cast(BME280_ADDR::w_CTRL_MEAS_ADDR));
            SPI.transfer(cast(controller.ControlValue));
        end_spi1(controller.CS_PIN);

    }

    void bme280_read_measurements(BME280_t& controller) {
        // As long all measurements are read in the same SPI call
        // or in burst as the datasheet calls it
        // it avoid shadowing which is the effect of reading
        // two measurements when they just happen to overlap.
        /// Temperature measurement
        int32_t bme_temp_register = 0;
        int32_t bme_pressure_register = 0;
        int32_t bme_hum_register = 0;
        // Returns 19:12 of the temp meas
        start_spi1(controller.CS_PIN, SPI_MODE0);
            // Start addr = 0xFA
            SPI.transfer(cast(BME280_ADDR::r_MEAS_ADDR));

            /// Pressure measurement
            bme_pressure_register = SPI.transfer(0xFF) << 12; // Returns 19:12 of the pressure meas
            bme_pressure_register |= SPI.transfer(0xFF) << 4; // Returns 11:4 of the pressure meas
            bme_pressure_register |= (0xF0 & SPI.transfer(0xFF)) >> 4; // Returns 3:0 of the pressure meas
            /// !Pressure measurement

            // Returns 19:12 of the temp meas
            bme_temp_register = SPI.transfer(0xFF) << 12;
            // Returns 11:4 of the temp meas
            bme_temp_register |= SPI.transfer(0xFF) << 4;
            // Returns 3:0 of the temp meas
            bme_temp_register |= (0xF0 & SPI.transfer(0xFF)) >> 4;
            /// !Temperature measurement

            /// Humidity measurement
            bme_hum_register = SPI.transfer(0xFF) << 8; // Returns 15:8 of the hum meas
            bme_hum_register |= SPI.transfer(0xFF); // LSB bits of the hum meas
            /// !Humidity measurement
        end_spi1(controller.CS_PIN);

        take_reg_mux();
            controller.REGISTERS.LAST_TEMP_REG = bme_temp_register;
            controller.REGISTERS.LAST_PRESSURE_REG = bme_pressure_register;
            controller.REGISTERS.LAST_HUM_REG = bme_hum_register;
        give_reg_mux();
    }

    void bme280_check_status(BME280_t&) {

    }

} // namespace SBCQueens