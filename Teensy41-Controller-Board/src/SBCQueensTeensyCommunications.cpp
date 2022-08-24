#include "SBCQueensTeensyCommunications.h"

#include <CRC32.h>

#include "SBCQueensTeensyHardware.h"

namespace SBCQueens {

    bool check_and_run(const uint32_t& crc_hash, const float& newVal) {
        
        for(size_t i = 0; i < (sizeof(CMD_HASH_MAP) / sizeof(CMDHash)); i++) {
            if(CMD_HASH_MAP[i].Hash == crc_hash) {
                CMD_HASH_MAP[i].Handler(newVal);
                return true;
            }
        }

        return false;
        
    }

    void CHECKERR(float newVal) {

    }

    void GETERR(float newVal) {

    }

    void RESET(float newVal) {

    }

    void SET_PPID(float newVal) {
        if(newVal > 0) {
            PELTIER_PID.State = PID_STATE::CURRENT_MODE;
        } else {
            PELTIER_PID.State = PID_STATE::DISABLE;
        }
        
    }

    void SET_PTKP_PID(float newVal) {
        PELTIER_PID.REGISTERS.CONTROL_KP = newVal;
    }

    void SET_PTTd_PID(float newVal) {
        PELTIER_PID.REGISTERS.CONTROL_TD = newVal;
    }

    void SET_PTTi_PID(float newVal) {
        PELTIER_PID.REGISTERS.CONTROL_TI = newVal;
    }

    void SET_PTEMP(float newVal) {
        PELTIER_PID.REGISTERS.DESIRED_CONTROL_VAL = newVal;
    }

    void RESET_PPID(float) {
        TCPID_restart(PELTIER_PID);
    }


    void GET_PRESSURES(float newVal) {
        char num_buff[11] = "";
        uint8_t length = 0;
        Serial.print("{");

        float f_send_vals[] = {
            VACUUM_PRESSURE_SENSOR.LATEST_VALUE
        };

        const char* names[] = {
            "\"VACUUMP\":"
        };

        const uint16_t size_f_send_vals = (sizeof(f_send_vals) / sizeof(float));
        for(unsigned int i = 0; i < size_f_send_vals; i++) {

            // Send the name first
            length = strlen(names[i]);
            Serial.print(names[i]);

            sprintf(num_buff, "%.6f", f_send_vals[i]);
            length = strlen(num_buff);
            Serial.print(num_buff);

            if((size_f_send_vals - 1) != i) {
                Serial.print(",");
            }
        }

        Serial.println("}");
    }

    void GET_RTDS(float newVal) {
        char num_buff[10] = "";
        uint8_t length = 0;
        Serial.print("{");

#ifdef NEW_RTD_BOARD
        float f_send_vals[NUM_RTD_BOARDS * NUM_RTD_PER_BOARD];

        for(uint8_t i = 0; i < NUM_RTD_BOARDS; i++) {
            for(uint8_t j = 0; j < NUM_RTD_PER_BOARD; j++) {
                f_send_vals[i] = RTD_BOARDS[i].LAST_TEMP_VAL[j];
            }
        }

        Serial.print("\"RTDT\":[");
#else
        float f_send_vals[] = {
            RTD_DAC_01.REGISTERS.LAST_TEMP_REG,
            RTD_DAC_02.REGISTERS.LAST_TEMP_REG
        };

        const char* names[] = {
            "\"RTDT1\":",
            "\"RTDT2\":",
        };
#endif

        const uint16_t size_f_send_vals = (sizeof(f_send_vals) / sizeof(float));
        for(unsigned int i = 0; i < size_f_send_vals; i++) {

#ifndef NEW_RTD_BOARD
            // Send the name first
            length = strlen(names[i]);
            Serial.print(names[i]);
#endif

            // Now the values
            // We limiting the sending of the values to 4 digits after the decimal point
            // to optimize for sending speed.
            // For temp RTDs, datasheet states 0.03125 degC
            // 4 is more than enough to send but internally is full precision
            sprintf(num_buff, "%.4f", f_send_vals[i]);
            length = strlen(num_buff);
            Serial.print(num_buff);

            if((size_f_send_vals - 1) != i) {
                Serial.print(",");
            }
            
        }

#ifdef NEW_RTD_BOARD
        Serial.print("]");
#endif
        Serial.println("}");
    }

    void GET_RAW_RTDS(float) {
        char num_buff[10] = "";
        uint8_t length = 0;
        Serial.print("{");

#ifdef NEW_RTD_BOARD
        uint16_t i_send_vals[NUM_RTD_BOARDS*NUM_RTD_PER_BOARD];

        for(uint8_t i = 0; i < NUM_RTD_BOARDS; i++) {
            for(uint8_t j = 0; j < NUM_RTD_PER_BOARD; j++) {
                i_send_vals[i] = RTD_BOARDS[i].LAST_ADC_VAL[j];
            }
        }

        Serial.print("\"RTDR\":[");

        const uint16_t size_i_send_vals = sizeof(i_send_vals) / sizeof(uint16_t);
        for(unsigned int i = 0; i < size_i_send_vals; i++) {

            Serial.print(i_send_vals[i]);

            if((size_i_send_vals - 1) != i) {
                Serial.print(",");
            }
            
        }

        Serial.print("]");
#endif
        Serial.println("}");
    }

    // This function must be used only inside the mutex safe region
    // of the serial task. GET_PIDS;2938400538
    void GET_PELTIERS_CURRS(float newVal) {
        char num_buff[10] = "";
        uint8_t length = 0;
        Serial.print("{");

        float f_send_vals[] = {
            PELTIER_DRIVER.REGISTERS.LAST_CURRENT_REG,
        };

        const char* names[] = {
            "\"PELTIER1I\":",
        };

        const uint16_t size_f_send_vals = (sizeof(f_send_vals) / sizeof(float));
        for(unsigned int i = 0; i < size_f_send_vals; i++) {

            // Send the name first
            length = strlen(names[i]);
            Serial.print(names[i]);

            // Now the values
            // We limiting the sending of the values to 4 digits after the decimal point
            // to optimize for sending speed.
            // The logic behind choosing 4 is that
            // for current, we have 12 bits, max 6.2A / 2^12 approx 1.5mA
            sprintf(num_buff, "%.4f", f_send_vals[i]);
            length = strlen(num_buff);
            Serial.print(num_buff);

            if((size_f_send_vals - 1) != i) {
                Serial.print(",");
            }
        }

        Serial.println("}");
    }

    //GET_BMES;1260808595
    void GET_BMES(float newVal) {
        char num_buff[11] = "";
        uint8_t length = 0;

        Serial.print("{");

        int32_t i_send_vals[] {
            LOCAL_BME280.REGISTERS.LAST_TEMP_REG,
            LOCAL_BME280.REGISTERS.LAST_PRESSURE_REG,
            LOCAL_BME280.REGISTERS.LAST_HUM_REG
        };

        const char* names[] = {
            "\"BME1T\":",
            "\"BME1P\":",
            "\"BME1H\":"
        };

        const uint16_t size_i_send_vals = (sizeof(i_send_vals) / sizeof(int32_t));
        for(unsigned int i = 0; i < size_i_send_vals; i++) {

            // Send the name first
            length = strlen(names[i]);
            Serial.print(names[i]);

            // We sending integers so I will take a bet and say this is pretty
            // efficient already and we do not have to limit anything
            snprintf(num_buff, 11, "%ld", i_send_vals[i]);
            length = strlen(num_buff);
            Serial.print(num_buff);
            
            if((size_i_send_vals - 1) != i) {
                Serial.print(",");
            }
        }

        Serial.println("}");
    }

    void SET_PELTIER_RELAY(float newVal) {
        if(newVal == 0) {
            digitalWrite(TWELVEV_PIN_ONE, arduino::LOW);
        } else {
            digitalWrite(TWELVEV_PIN_ONE, arduino::HIGH);
        }
    }

    
} // namespace SBCQueens