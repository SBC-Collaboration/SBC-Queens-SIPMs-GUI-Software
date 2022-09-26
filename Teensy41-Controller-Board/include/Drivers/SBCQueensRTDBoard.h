#pragma once

#include <inttypes.h>

#include "SBCQueensTeensyBasics.h"

namespace SBCQueens {

    const uint8_t c_MCP23S08_ADDR = 0b01000000;
    const uint8_t c_MCP23S08_GPIO = 0x09;
    const uint8_t c_MCP23S08_IODIR = 0x00;

    template<size_t N = 3>
    struct RTDBoard {
        // RTD MASK, if bit n is 1, that RTD is enabled;
        uint8_t MASK = 0xFF;
        uint8_t MCP23S08_CS;

        // ADC is LTC2452
        uint8_t ADC_CS[N];
        uint16_t LAST_ADC_VAL[N] = {0};
        float LAST_TEMP_VAL[N];
    };

    template<size_t N>
    // Initializes the I/O expander for all outputs and sets them to 0
    void RTDBoard_init(RTDBoard<N>& controller) {

        pinMode(controller.MCP23S08_CS, arduino::OUTPUT);
        digitalWrite(controller.MCP23S08_CS, arduino::HIGH);

        for(size_t i = 0; i < N; i++) {
            pinMode(controller.ADC_CS[i], arduino::OUTPUT);
            digitalWrite(controller.ADC_CS[i], arduino::HIGH);
        }


        //starting the transaction
        start_spi(controller.MCP23S08_CS, SPI_MODE0);
        //addressing the big bug chip
        SPI.transfer(c_MCP23S08_ADDR);
        //setting the specific value to send
        
        //sending it
        SPI.transfer(c_MCP23S08_IODIR);
        SPI.transfer(0x00);
        // deselecting the pin and then ending the transaction
        end_spi(controller.MCP23S08_CS);


        start_spi(controller.MCP23S08_CS, SPI_MODE0);
        SPI.transfer(c_MCP23S08_ADDR);

        const uint8_t c_MCP23S08_IOCON = 0x05;
        SPI.transfer(c_MCP23S08_IOCON);
        SPI.transfer(0b00011000);
        end_spi(controller.MCP23S08_CS);

        start_spi(controller.MCP23S08_CS, SPI_MODE0);
        SPI.transfer(c_MCP23S08_ADDR);
        SPI.transfer(c_MCP23S08_GPIO);
        SPI.transfer(0xFF);

        // equivalent to SPI.endTransaction();
        end_spi(controller.MCP23S08_CS);
    }

    template<size_t N>
    // Turns on the current sources. 
    // Allow the RTD to heat up (1ms or more) before triggering a measurement
    void RTDBoard_prepare_meas(RTDBoard<N>& controller) {
        start_spi(controller.MCP23S08_CS, SPI_MODE0);
        SPI.transfer(c_MCP23S08_ADDR);
        SPI.transfer(c_MCP23S08_GPIO);
        SPI.transfer(0x00);
        end_spi(controller.MCP23S08_CS);
    }

    template<size_t N>
    // Takes last measurement and triggers a new one. 
    // Do not turn off the current source right away. A measurement takes 16.6ms (typical)
    void RTDboard_take_meas(RTDBoard<N>& controller) {

        for (size_t i = 0; i < N; i++) {
            take_reg_mux();
            start_spi(controller.ADC_CS[i], SPI_MODE0);

            controller.LAST_ADC_VAL[i] = 0;
            //The  #s don't matter here, we just need to transfer literally anything
            controller.LAST_ADC_VAL[i] = SPI.transfer(2) << 8;
            controller.LAST_ADC_VAL[i] |= SPI.transfer(4);


            end_spi(controller.ADC_CS[i]);
            give_reg_mux();

            // configTICK_RATE_HZ = 50kHz
            // 1 / portTICK_PERIOD_MS = 50us
            //vTaskDelay( 1 / portTICK_PERIOD_MS );
        }
        
    }

    template<size_t N>
    // Puts the current sources into standby/off state
    void RTDboard_standby(RTDBoard<N>& controller) {

        start_spi(controller.MCP23S08_CS, SPI_MODE0);
        SPI.transfer(c_MCP23S08_ADDR);
        SPI.transfer(c_MCP23S08_GPIO);
        SPI.transfer(0xFF);
        end_spi(controller.MCP23S08_CS);

    }


    float __rtd_reg_to_temperature(const uint16_t& Rt);

    template<size_t N>
    void RTDboard_translate_meas(RTDBoard<N>& controller) {
        
        float tmp[N] = {0};
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