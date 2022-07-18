#pragma once

#include <inttypes.h>

namespace SBCQueens {
	// TeensyController registers
	// They might or not have relationship with internal registers
	// Or read control bytes
	struct MAX31865Registers_t {
		float LAST_TEMP_REG  	= 0.0;
		uint8_t ERROR 			= 0;
	};

	enum class MAX31865_ADDR {
		r_CONF_ADDR 	= 0x00,
		w_CONF_ADDR 	= 0x80,
		r_TEMP_ADDR 	= 0x01,
		r_FAULT_ADDR 	= 0x07
	};

	// This holds the 3 MSB of the configuration register
	// 7 MSB, 	Vbias.				1 = ON; 0 = OFF
	// 5,		1-shot.				1 = 1-shot (auto clear)
	// 3:2,		Fault detection		See table in datasheet
	const uint8_t MAX31865_STATES_MASK = 0b10101100;
	enum class MAX31865_STATES {
		PREPARE_MEAS_VAL 		= 0b10000000,
		ENABLE_ONE_SHOT 		= 0b10100000,
		SLEEP 					= 0x00,
		START_FAULT_STATE_1 	= 0b10001000,
		START_FAULT_STATE_2 	= 0b10001100,
	};

	// This holds the rest bits of the configuration register
	// 6,		Conversion mode.	1 = Auto; 0 = Normally off
	// 4,		3-wire.				1 = 3-wire; 0 = 2 or 4 wire
	// 1,		Fault status		1 = Clear (auto clear)
	// 0,		Filter select		1 = 50Hz; 0 = 60Hz
	const uint8_t MAX31865_CONF_VALS_MASK = 0b01010011;
	enum class MAX31865_CONF_VALS {
		PID_OPTIMIZED 			= 0x00,
		DEFAULT_STATE			= 0b01010000
	};

	struct MAX31865_t {

		uint8_t CS_PIN;
		MAX31865Registers_t 	REGISTERS;
		
		MAX31865_STATES 		State 			= MAX31865_STATES::SLEEP;
		MAX31865_CONF_VALS 		Configuration 	= MAX31865_CONF_VALS::DEFAULT_STATE;

	};

	void max31865_prepare_measurement(MAX31865_t&);
	void max31865_start_measurement(MAX31865_t&);
	void max31865_retrieve_measurement(MAX31865_t&);

	void max31865_start_fault_detection(MAX31865_t&);
	void max31865_finish_fault_detection(MAX31865_t&);

	/// Math specific functions
	const float c_RTD_NOMINAL = 100.0; 
	const float c_REF_RESISTOR = 430.0;
	const float c_RTD_A = 3.9083e-3;
	const float c_RTD_B = -5.775e-7;

	const float c_Z1 = -c_RTD_A;
	const float c_Z2 = c_RTD_A * c_RTD_A - (4 * c_RTD_B);
	const float c_Z3 = (4 * c_RTD_B) / c_RTD_NOMINAL;
	const float c_Z4 = 2 * c_RTD_B;

	float __register_to_temperature(const uint16_t& Rt);
	
} // namespace SBCQueens
