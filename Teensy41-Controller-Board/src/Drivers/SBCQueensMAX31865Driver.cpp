#include "Drivers/SBCQueensMAX31865Driver.h"

#include <math.h>

#include "SBCQueensTeensyBasics.h"

namespace SBCQueens {

  void max31865_prepare_measurement(MAX31865_t& controller) {

    start_spi(controller.CS_PIN, SPI_MODE1);
      // Write Conf register
      SPI.transfer(cast(MAX31865_ADDR::w_CONF_ADDR)); 
      // Enables Vbias
      SPI.transfer(cast(MAX31865_STATES::PREPARE_MEAS_VAL) | cast(controller.Configuration)); 
    end_spi(controller.CS_PIN);

  }

  void max31865_start_measurement(MAX31865_t& controller) {

    start_spi(controller.CS_PIN, SPI_MODE1);    
      SPI.transfer(cast(MAX31865_ADDR::w_CONF_ADDR));
      SPI.transfer(cast(MAX31865_STATES::ENABLE_ONE_SHOT) | cast(controller.Configuration)); 
    // When ending SPI, RTD_1_CS should go high which starts the measurement
    end_spi(controller.CS_PIN);

  }

  void max31865_retrieve_measurement(MAX31865_t& controller) {

    uint16_t latest_temp_reg = 0;
    uint8_t tmp_error = 0;

    start_spi(controller.CS_PIN, SPI_MODE1);
      SPI.transfer(cast(MAX31865_ADDR::r_TEMP_ADDR));
      // MSB of resulting measurement
      latest_temp_reg = SPI.transfer(0xFF) << 8;
      // LSB
      latest_temp_reg |= SPI.transfer(0xFF);
    end_spi(controller.CS_PIN);

    // Gets the fault and saved to error
    tmp_error = latest_temp_reg & 0x01; 
    // This removes the fault
    latest_temp_reg >>= 1;

    // After the measurement, we turn everything off and send the sensor back to sleep
    start_spi(controller.CS_PIN, SPI_MODE1);
      SPI.transfer(cast(MAX31865_ADDR::w_CONF_ADDR));
      SPI.transfer(cast(MAX31865_STATES::SLEEP) | cast(controller.Configuration));
    // We are done with the temperature sensor routine
    end_spi(controller.CS_PIN);


    float temp_measurement = __register_to_temperature(latest_temp_reg);
    take_reg_mux();
      controller.REGISTERS.LAST_TEMP_REG = temp_measurement;
      controller.REGISTERS.ERROR |= tmp_error;
    give_reg_mux();
  }

  void max31865_start_fault_detection(MAX31865_t* controller) {

  }

  void max31865_finish_fault_detection(MAX31865_t* controller) {

  }

  // This function is not mine, I took it from
  // https://github.com/adafruit/Adafruit_MAX31865/blob/master/Adafruit_MAX31865.cpp
  // but I have modified it slightly
  float __register_to_temperature(const uint16_t& Rt) {
    float temp, Rtf;

    Rtf = static_cast<float>(Rt);
    Rtf /= 32768;
    Rtf *= c_REF_RESISTOR;

    // Serial.print("\nResistance: "); Serial.println(Rt, 8);
    temp = c_Z2 + (c_Z3 * Rtf);
    temp = (sqrt(temp) + c_Z1) / c_Z4;

    if (temp >= 0.0)
      return temp;

    // ugh.
    Rtf /= c_RTD_NOMINAL;
    Rtf *= 100; // normalize to 100 ohm

    float rpoly = Rtf;

    temp = -242.02;
    temp += 2.2228 * rpoly;
    rpoly *= Rtf; // square
    temp += 2.5859e-3 * rpoly;
    rpoly *= Rtf; // ^3
    temp -= 4.8260e-6 * rpoly;
    rpoly *= Rtf; // ^4
    temp -= 2.8183e-8 * rpoly;
    rpoly *= Rtf; // ^5
    temp += 1.5243e-10 * rpoly;

    return temp;
  }
  
} // namespace SBCQueens