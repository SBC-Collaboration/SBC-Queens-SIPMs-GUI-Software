#pragma once

#include <inttypes.h>

namespace SBCQueens {

    const uint8_t c_MCP23S08_ADDR = 0b01000000;
    const uint8_t c_MCP23S08_GPIO = 0x09;

    struct RTDBoard {
        uint8_t MCP23S08_CS;
        uint8_t ADC_CS;

        uint16_t LAST_ADC_VAL;
    };

    void RTDBoard_init(RTDBoard&);
    void RTDBoard_prepare_meas(RTDBoard&);
    void RTDboard_take_meas(RTDBoard&);
    void RTDboard_retrieve_meas(RTDBoard&);


}; // namespace SBCQueens