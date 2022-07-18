#include "Drivers/SBCQueensRTDBoard.h"

#include <arduino_freertos.h>

#include "SBCQueensTeensyBasics.h"

namespace SBCQueens {

    void RTDBoard_init(RTDBoard& controller) {

        pinMode(controller.MCP23S08_CS, arduino::OUTPUT);
        pinMode(controller.ADC_CS, arduino::OUTPUT);

        digitalWrite(controller.MCP23S08_CS, arduino::HIGH);
        digitalWrite(controller.ADC_CS, arduino::HIGH);

        start_spi(controller.MCP23S08_CS);
        // equivalent to SPI.beginTransaction();
        SPI.transfer(c_MCP23S08_ADDR);
        const uint8_t c_MCP23S08_IODIR = 0x00;
        SPI.transfer(c_MCP23S08_IODIR);
        SPI.transfer(0x00);
        // equivalent to SPI.endTransaction();
        end_spi(controller.MCP23S08_CS);


        start_spi(controller.MCP23S08_CS);
        // equivalent to SPI.beginTransaction();
        SPI.transfer(c_MCP23S08_ADDR);

        const uint8_t c_MCP23S08_IOCON = 0x05;
        SPI.transfer(c_MCP23S08_IOCON);

        SPI.transfer(0b00001000);

        // equivalent to SPI.endTransaction();
        end_spi(controller.MCP23S08_CS);

        
        start_spi(controller.MCP23S08_CS);
        // equivalent to SPI.beginTransaction();
        SPI.transfer(c_MCP23S08_ADDR);
        SPI.transfer(c_MCP23S08_GPIO);
        SPI.transfer(0x00);

        // equivalent to SPI.endTransaction();
        end_spi(controller.MCP23S08_CS);

    }

    void RTDBoard_prepare_meas(RTDBoard& controller) {

        start_spi(controller.MCP23S08_CS);

        SPI.transfer(c_MCP23S08_ADDR);
        SPI.transfer(c_MCP23S08_GPIO);
        SPI.transfer(0b00000010);

        end_spi(controller.MCP23S08_CS);


    }

    void RTDboard_take_meas(RTDBoard& controller) {

        start_spi(controller.MCP23S08_CS);

        SPI.transfer(c_MCP23S08_ADDR);
        SPI.transfer(c_MCP23S08_GPIO);
        SPI.transfer(0b00000000);

        end_spi(controller.MCP23S08_CS);

    }

    void RTDboard_retrieve_meas(RTDBoard& controller) {

    }

}; // namespace SBCQueens