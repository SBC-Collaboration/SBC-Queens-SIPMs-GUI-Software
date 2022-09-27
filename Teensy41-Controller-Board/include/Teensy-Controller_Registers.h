#pragma once

#include <inttypes.h>

struct PID_REGISTERS
{
	// All of these regs save the values
	// directly of all of the items read
	// None of them are higher than 16 bits
	float LAST_TEMP_REG;
	float DESIRED_TEMP_REG;
	float LAST_CURRENT_REG;
	uint16_t LAST_DAC_REG;

	// Holds the error of the temp sensor, watt meter
	// and DAC. Order to be defined
	uint16_t PID_STATUS_REG;
	// For now only states if its in CC (constant current mode)
	// or in CT (constant temperature mode)
	uint16_t PID_STATE_REG; 
};


// PC comm pointers
// from most significant bit to lower:
// XXXX XXXX XXX SET_DES_TEMP[1] RESET_ERROR[1] TOGGLE_STATE[1] SEND_HT[1] READ_HUMIDIY[1]
// READ_HUMIDIY[1] -> Starts a hum measurement
// SEND_HT[1] -> Send a temperature (RTD) and humidity measurement
// TOGGLE_STATE[1] -> Toggles between STANDBY and RUNNING
// RESET_ERROR[1] -> Resets error part of the status flag


#define READ_HUMIDIY_BIT 0
#define SEND_HT_BIT 1
#define TOGGLE_STATE_BIT 2
#define RESET_ERROR_BIT 3
#define SET_DES_BIT 4
// static uint16_t COMMAND_REGISTER;
// Saves the current temperature


// from most significant bit to lower:
// XXXX STATUS[1] DAC_ERR[1] DHT_ERR[1] VT_ERR[1] RTD_ERR[8]  
// No error checking for the ina219 (wattmeter) available
#define STATUS_BIT 11
#define DAC_ERR_BIT 10
#define DHT_ERR_BIT 9
#define VT_ERR_BIT 8
#define RTD_ERR 0
// static uint16_t STATUS_FLAG;


#define DAC_ADDR 0b1100000
#define ADC_ADDR 0b1001000