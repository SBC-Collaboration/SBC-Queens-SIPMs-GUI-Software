#include "SBCQueensTeensyCommunications.h"

#include <CRC32.h>

#include "SBCQueensTeensyHardware.h"
#include "SBCQueensTiming.h"

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

    void SET_PPID_UP(float newVal) {
        PIDUpdateTime = static_cast<uint16_t>(newVal);

        if(PIDUpdateTime < RTDSamplingTime) {
            PIDUpdateTime = RTDSamplingTime;
        }

        xTimerChangePeriod(update_pid_h,
            pdMS_TO_TICKS(PIDUpdateTime),
            portMAX_DELAY);

        PELTIER_PID.DeltaTime = PIDUpdateTime;
    }

    void SET_PPID_RTD(float newVal) {
        uint16_t index = static_cast<uint16_t>(newVal);

        uint16_t board_index = index / NUM_RTD_PER_BOARD;
        // Ceil it between 0 and NUM_RTD_BOARDS
        board_index = board_index < 0 ? 0 : board_index;
        board_index = board_index >= NUM_RTD_BOARDS ? NUM_RTD_BOARDS - 1 : board_index;

        uint16_t rtd_index = index % NUM_RTD_PER_BOARD;
    #ifdef NEW_RTD_BOARD
        PELTIER_PID.REGISTERS.LATEST_CONTROL = &RTD_BOARDS[board_index].LAST_TEMP_VAL[rtd_index];
    #else 
        PELTIER_PID.REGISTERS.LATEST_CONTROL = &RTD_BOARDS[board_index].REGISTERS.LAST_TEMP_REG;
    #endif
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

    void SET_RTD_SP(float newVal ) { 
        RTDSamplingTime = static_cast<uint16_t>(newVal);

        // Ceil it to the constants. It should never be faster than this!
        if ( RTDSamplingTime < c_RTDAcquisitionTime + c_RTDPrepareTime + 1) {
            RTDSamplingTime = c_RTDAcquisitionTime + c_RTDPrepareTime + 1;
        }

        xTimerChangePeriod(prepare_rtd_conversion_h,
            pdMS_TO_TICKS(RTDSamplingTime), 
            portMAX_DELAY);
    }

    void RTD_BANK_MASK(float newVal) {
    #ifdef NEW_RTD_BOARD
        auto mask = static_cast<uint32_t>(newVal);

        for(uint8_t i = 0; i < SBCQueens::NUM_RTD_BOARDS; i++) {
            // 2^n = ...0001000...
            // the one will be in the n position, so if we subtract by 1
            // 2^n - 1 = ...0000111...
            // we get the mask we need!
            uint8_t individual_mask = exp2(NUM_RTD_PER_BOARD) - 1;
            // We then shift it depending on how many RTD boards we have
            // These are the bits that correspond to the rtd board i with n rtds
            individual_mask = individual_mask << NUM_RTD_PER_BOARD*i;

            // So all is left is to mask this to the real mask and shifted back. 
            RTD_BOARDS[i].MASK = (mask & individual_mask) >> NUM_RTD_PER_BOARD*i;

            // To make these operations clearer, lets assume we have 2 boards (i = 2) with 2 rtds (n = 2)
            // and we send a mask with binary representation equal to (ignoring the first 24 MSB)
            //      0b00001010
            // Means we want the 2nd RTD of each board to be one
            // The individual mask for each board is 
            //      (2^2 - 1 << 0) = 0b00000011 for board 1
            //      (2^2 - 1 << 2) = 0b00001100 for board 2
            // So then the mask for board 1 is 
            //      0b000000011 & 0b0001010 = 0b00000010 (no shift as X >> 0 = X)
            // the mask for board 2 is
            //      0b000001100 & 0b0001010 = 0b00001000 (but needs to be shifted)
            //      0b00001000 >> 2 = 0b00000010
            //
            // Hope this made it clearer.
        }   
    #endif
    }

    void GET_SYS_PARAMETERS(float) {
        Serial.print("{");
        Serial.print("\"NUM_RTD_BOARDS\":");
        Serial.print(NUM_RTD_BOARDS);
        Serial.print(",");

        Serial.print("\"NUM_RTDS_PER_BOARD\":");
        Serial.print(NUM_RTD_PER_BOARD);
        Serial.print(",");

        Serial.print("\"RTD_ONLY_MODE\":");
    #ifdef RTD_ONLY_MODE
        Serial.print("true");
    #else
        Serial.print("false");
    #endif;
        Serial.println("}");
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

        
#else
        float f_send_vals[NUM_RTD_BOARDS * NUM_RTD_PER_BOARD];
        for(uint8_t i = 0; i < NUM_RTD_BOARDS; i++) {
            for(uint8_t j = 0; j < NUM_RTD_PER_BOARD; j++) {
                f_send_vals[i] = RTD_BOARDS[i].REGISTERS.LAST_TEMP_REG;
            }
        }
#endif

        Serial.print("\"RTDT\":[");

        const uint16_t size_f_send_vals = (sizeof(f_send_vals) / sizeof(float));
        for(unsigned int i = 0; i < size_f_send_vals; i++) {

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

        Serial.print("]");
        Serial.println("}");
    }

    void GET_RAW_RTDS(float) {
        char num_buff[10] = "";
        uint8_t length = 0;
        Serial.print("{");

#ifdef NEW_RTD_BOARD
        uint16_t i_send_vals[NUM_RTD_BOARDS*NUM_RTD_PER_BOARD] = {0};

        for(uint8_t i = 0; i < NUM_RTD_BOARDS; i++) {
            for(uint8_t j = 0; j < NUM_RTD_PER_BOARD; j++) {
                i_send_vals[NUM_RTD_PER_BOARD*i + j] = RTD_BOARDS[i].LAST_ADC_VAL[j];
            }
        }

        Serial.print("\"RTDR\":[");

        const uint16_t size_i_send_vals = NUM_RTD_BOARDS*NUM_RTD_PER_BOARD;
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
