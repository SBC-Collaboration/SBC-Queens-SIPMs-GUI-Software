#pragma once

#include <inttypes.h>
#include <string.h>
#include <vector>
#include <utility>
#include <string>

namespace SBCQueens {
    // A quick hash function that is totally not mine. I stole it from
    // CRC32 arduino library.
    constexpr uint32_t c_crc32_checksum(const char* data)
    {
        uint32_t crc32_table[] = {
            0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
            0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
            0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
            0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
        };

        size_t nBytes = strlen(data) * sizeof(char);
        uint8_t tbl_idx = 0;
        uint32_t _state = ~0L;

        for (size_t i = 0; i < nBytes; i++)
        {
            tbl_idx = 0;

            tbl_idx = _state ^ (data[i] >> (0 * 4));
            _state = *(uint32_t*)(crc32_table + (tbl_idx & 0x0f)) ^ (_state >> 4);
            tbl_idx = _state ^ (data[i] >> (1 * 4));
            _state = *(uint32_t*)(crc32_table + (tbl_idx & 0x0f)) ^ (_state >> 4);
        }


        return ~_state;
    }

    const char c_ACK_str[] = "ACK;\n";
    const char c_NACK_str[] = "NACK;\n";
    const char c_INVALID_CMD_str[] = "INVALID CMD;\n";

    struct CMDHash {

        uint32_t Hash;
        void (*Handler)(float);

    };

    // This automatically grabs the name of the function, uses it to
    // feed it into a crc32 encoder which then turns into a hash key
    
    #define CMD_KEY(__name) {c_crc32_checksum( #__name ), __name}

    void CHECKERR(float);
    void GETERR(float);
    void RESET(float);

    void SET_PPID(float);
    void SET_PTKP_PID(float);
    void SET_PTTd_PID(float);
    void SET_PTTi_PID(float);
    void SET_PTEMP(float);
    void RESET_PPID(float);

    void GET_PRESSURES(float);
    void GET_RTDS(float);
    void GET_RAW_RTDS(float);
    void GET_PELTIERS_CURRS(float);
    void GET_BMES(float);
    void SET_PELTIER_RELAY(float);

    static CMDHash CMD_HASH_MAP[]  = {
        /// General system commands
        CMD_KEY(CHECKERR),
        CMD_KEY(GETERR),
        CMD_KEY(RESET),
        /// !General system commands
        ////
        /// Hardware specific commands
        CMD_KEY(SET_PPID),       // Turns ON or OFF this PID
        CMD_KEY(SET_PTKP_PID),   // Sets Kp for PID temperature control loop
        CMD_KEY(SET_PTTd_PID),   // Sets Td for PID temperature control loop
        CMD_KEY(SET_PTTi_PID),   // Sets Ti for PID temperature control loop
        CMD_KEY(SET_PTEMP),      // Sets PID temperature setpoint
        CMD_KEY(RESET_PPID),      // Reset internal PID variables

        //// Getters
        CMD_KEY(GET_PRESSURES),
        CMD_KEY(GET_RTDS),
        CMD_KEY(GET_RAW_RTDS),      // Gets the RTD register values
        CMD_KEY(GET_PELTIERS_CURRS),// Gets all the local PID registers. Avoids shadowing
        CMD_KEY(GET_BMES),          // Gets the BME registers. Avoids shadowing
   
        CMD_KEY(SET_PELTIER_RELAY)
        
        /// !Hardware specific commands
    };

    // Takes in the serial cmd hash, looks for the corresponding function
    // and runs it
    bool check_and_run(const uint32_t& crc_hash, const float& newVal);

} // namespace SBCQueens