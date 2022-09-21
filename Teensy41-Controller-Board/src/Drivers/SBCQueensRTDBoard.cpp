#include "Drivers/SBCQueensRTDBoard.h"

#include <arduino_freertos.h>

#include "SBCQueensTeensyBasics.h"

namespace SBCQueens {

    float __rtd_reg_to_temperature(const uint16_t& Rt) {
        const float c_RTD_NOMINAL = 100.0;
        const float c_REF_RESISTOR = 120.0;
        const float c_RTD_A = 3.9083e-3;
        const float c_RTD_B = -5.775e-7;

        const float c_Z1 = -c_RTD_A;
        const float c_Z2 = c_RTD_A * c_RTD_A - (4 * c_RTD_B);
        const float c_Z3 = (4 * c_RTD_B) / c_RTD_NOMINAL;
        const float c_Z4 = 2 * c_RTD_B;
        
        float temp, Rtf;
        Rtf = c_REF_RESISTOR*((Rt / 32768.0f) - 1.0f);

        temp = c_Z2 + (c_Z3 * Rtf);
        temp = (sqrt(temp) + c_Z1) / c_Z4;

        // if (temp >= 0.0)
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

}; // namespace SBCQueens