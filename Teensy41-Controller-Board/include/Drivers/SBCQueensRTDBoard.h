#pragma once

#include <inttypes.h>

#include "SBCQueensTeensyBasics.h"

namespace SBCQueens {

    const uint8_t c_MCP23S08_ADDR = 0b01000000;
    const uint8_t c_MCP23S08_GPIO = 0x09;
    const uint8_t c_MCP23S08_IODIR = 0x00;

    template<size_t N = 3>
    struct RTDBoard {
        uint8_t MCP23S08_CS;

        uint8_t ADC_CS[N];
        uint16_t LAST_ADC_VAL[N];
        float LAST_TEMP_VAL[N];
    };

    template<size_t N>
    void RTDBoard_init(RTDBoard<N>& controller) {

        pinMode(controller.MCP23S08_CS, arduino::OUTPUT);
        digitalWrite(controller.MCP23S08_CS, arduino::HIGH);

        for(size_t i = 0; i < N; i++) {
            pinMode(controller.ADC_CS[i], arduino::OUTPUT);
            digitalWrite(controller.ADC_CS[i], arduino::HIGH);
        }


        //starting the transaction
        start_spi(controller.MCP23S08_CS, SPI_MODE3);
        delayMicroseconds(1);
        //addressing the big bug chip
        SPI.transfer(c_MCP23S08_ADDR);
        //setting the specific value to send
        
        //sending it
        SPI.transfer(c_MCP23S08_IODIR);
        SPI.transfer(0x00);
        // deselecting the pin and then ending the transaction
        delayMicroseconds(1);
        end_spi(controller.MCP23S08_CS);


        start_spi(controller.MCP23S08_CS, SPI_MODE3);
        delayMicroseconds(1);
        SPI.transfer(c_MCP23S08_ADDR);

        const uint8_t c_MCP23S08_IOCON = 0x05;
        SPI.transfer(c_MCP23S08_IOCON);
        delayMicroseconds(1);
        end_spi(controller.MCP23S08_CS);

        start_spi(controller.MCP23S08_CS, SPI_MODE3);
        delayMicroseconds(1);
        SPI.transfer(c_MCP23S08_ADDR);
        SPI.transfer(c_MCP23S08_GPIO);
        SPI.transfer(0xFF);

        // equivalent to SPI.endTransaction();
        delayMicroseconds(1);
        end_spi(controller.MCP23S08_CS);
    }

    template<size_t N>
    void RTDBoard_prepare_meas(RTDBoard<N>& controller) {
        start_spi(controller.MCP23S08_CS, SPI_MODE3);
        delayMicroseconds(1);
        SPI.transfer(c_MCP23S08_ADDR);
        SPI.transfer(c_MCP23S08_GPIO);
        SPI.transfer(0xFF);
        delayMicroseconds(1);
        end_spi(controller.MCP23S08_CS);
    }

    template<size_t N>
    void RTDboard_take_meas(RTDBoard<N>& controller) {

        for (size_t i = 0; i < N; i++) {
            start_spi(controller.ADC_CS[i], SPI_MODE3);
            delayMicroseconds(1);

            take_reg_mux();
            controller.LAST_ADC_VAL[i] = 0;
            //The  #s don't matter here, we just need to transfer literally anything
            controller.LAST_ADC_VAL[i] = (SPI.transfer(2) & 0xEF) << 8;
            controller.LAST_ADC_VAL[i] |= SPI.transfer(4);
            give_reg_mux();

            delayMicroseconds(1);
            end_spi(controller.ADC_CS[i]);

            delayMicroseconds(25);
        }
        

        start_spi(controller.MCP23S08_CS, SPI_MODE3);
        delayMicroseconds(1);
        SPI.transfer(c_MCP23S08_ADDR);
        SPI.transfer(c_MCP23S08_GPIO);
        SPI.transfer(0x00);
        delayMicroseconds(1);
        end_spi(controller.MCP23S08_CS);


    }

    float __rtd_reg_to_temperature(const uint16_t& Rt);

    template<size_t N>
    void RTDboard_translate_meas(RTDBoard<N>& controller) {
        
        float tmp[N];
        for(size_t i = 0; i < N; i++) {
            tmp[i] = __rtd_reg_to_temperature(controller.LAST_ADC_VAL[i]);
        }

        take_reg_mux();
        for(size_t i = 0; i< N; i++) {
            controller.LAST_TEMP_VAL[i] = tmp[i];
        }
        give_reg_mux();
        
    }


    


}; // namespace SBCQueens