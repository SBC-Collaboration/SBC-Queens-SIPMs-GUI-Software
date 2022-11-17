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


    float temp_measurement = __register_to_temperature_90(latest_temp_reg);
    take_reg_mux();
      controller.REGISTERS.LAST_REG = latest_temp_reg;
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

  float __register_to_temperature_90(const uint16_t& Rt) {
    // Obtained from
    // https://us.flukecal.com/pt100-calculator
    // With T = 273.16K
    const double kRTDTRIPLEPOINT = 100.004;
    const double Rtf = __register_to_resistance(Rt);
    const double W = Rtf / kRTDTRIPLEPOINT;

    // These constants obtained from 
    // https://www.bipm.org/en/committees/cc/cct/guides-to-thermometry
    // Part 5, Pg 8
    const double D[] = {439.932854, 472.418020, 37.684494, 7.472018,
                      2.920828, 0.005184, -0.963864, -0.188732,
                      0.191203, 0.049025};

    double x = (W - 2.64f) / 1.64f;
    // This is the polynomial evaluation done in a more efficient way.
    // See https://en.wikipedia.org/wiki/Horner%27s_method
    // for more details
    double out = D[9];
    for(int i = 8; i >= 0; i--) {
      out = x*out + D[i];
    }

    // This equation is valid for 0.01degC or below
    if (out < 0.01) {
      // These constants obtained from 
      // https://www.bipm.org/en/committees/cc/cct/guides-to-thermometry
      // Part 5, Pg 8
      const double B[] = {0.183324722, 0.240975303, 0.209108711, 0.190439972,
                        0.142648498, 0.077993465, 0.012475611, -0.032267127,
                        -0.075291522, -0.056470670, 0.076201285, 0.123893204,
                        -0.029201193, -0.091173542, 0.001317696, 0.026025526};

      x = (pow(W, 1.0/6.0) - 0.65f) / 0.35f;
      out = B[15];
      for(int i = 14; i >= 0; i--) {
        out = x*out + B[i];
      }

      return static_cast<float>(273.16*out - 273.15);
    } else {
      return static_cast<float>(out);
    }

    return out;
  }

	float __register_to_resistance(const uint16_t& Rt) {
    float Rtf = static_cast<float>(Rt);
    Rtf /= 32768.0;
    Rtf *= c_REF_RESISTOR;
    return Rtf;
  }
  
} // namespace SBCQueens