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

    void SET_NPID(float newVal) {
        if(newVal > 0) {
            N2_PID.State = PID_STATE::TEMP_MODE;
        } else {
            N2_PID.State = PID_STATE::DISABLE;
        }
        
    }

    void SET_NTKP_PID(float newVal) {
        N2_PID.REGISTERS.CONTROL_KP = newVal;
    }

    void SET_NTTd_PID(float newVal) {
        N2_PID.REGISTERS.CONTROL_TD = newVal;
    }

    void SET_NTTi_PID(float newVal) {
        N2_PID.REGISTERS.CONTROL_TI = newVal;
    }

    void SET_NTEMP(float newVal) {
        N2_PID.REGISTERS.DESIRED_CONTROL_VAL = newVal;
    }

    void RESET_NPID(float) {
        TCPID_restart(N2_PID);
    }

    void SET_PSI_LIMIT(float newVal) {

    }

    void REL_VALVE_STATE(float newVal) {
        // digitalWrite(TWELVEV_PIN_TWO, newVal == 0);
    }

    void N2_VALVE_STATE(float newVal) {
        SBCQueens::N2_RELEASE_MAN_REG = (newVal > 0);
    }

    void GET_PRESSURES(float newVal) {
        char num_buff[11] = "";
        uint8_t length = 0;
        Serial.print("{");

        float f_send_vals[] = {
            VACUUM_PRESSURE_SENSOR.LATEST_VALUE,
            NTWO_PRESSURE_SENSOR.LATEST_VALUE
        };

        const char* names[] = {
            "\"VACUUMP\":",
            "\"NTWOP\":",
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

        float f_send_vals[] = {
            RTD_DAC_01.REGISTERS.LAST_TEMP_REG,
            RTD_DAC_02.REGISTERS.LAST_TEMP_REG
        };

        const char* names[] = {
            "\"RTDT1\":",
            "\"RTDT2\":",
        };

        const uint16_t size_f_send_vals = (sizeof(f_send_vals) / sizeof(float));
        for(unsigned int i = 0; i < size_f_send_vals; i++) {

            // Send the name first
            length = strlen(names[i]);
            Serial.print(names[i]);

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
            LOCAL_BME280.REGISTERS.LAST_HUM_REG,
            BOX_BME280.REGISTERS.LAST_TEMP_REG,
            BOX_BME280.REGISTERS.LAST_PRESSURE_REG,
            BOX_BME280.REGISTERS.LAST_HUM_REG
        };

        const char* names[] = {
            "\"BME1T\":",
            "\"BME1P\":",
            "\"BME1H\":",
            "\"BME2T\":",
            "\"BME2P\":",
            "\"BME2H\":"
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