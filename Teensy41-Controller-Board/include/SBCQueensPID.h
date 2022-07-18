#pragma once

#include <inttypes.h>

#include "Drivers/SBCQueensPeltierDriver.h"

/*

	This is my own hand written PID code.
	I had to do it myself because none of the available libraries fit
	my requirements: 
		1.- Work with floats
		2.- Assume fixed delta time that is known
		3.- Have the possibility to switch between two modes (SET and CONTROL),
			where CONTROL is the main variable to control and SET is 
			a probable intermediate step
		4.- Ceil the output values

	You can say this is an Teensy 4.1 optimized PID, too
	(float operations = same time as int operations)

*/

// For Peltier PID -> Control = Temperature [degC]
// set = current source [A]
namespace SBCQueens {
	struct PIDRegisters {

		float DESIRED_CONTROL_VAL 	= 0.0;  // Arb U1
		float DESIRED_SET_VAL		= 0.0;  // Arb U2

		float CONTROL_KP			= 0.0;  // Arb U1
		float CONTROL_TD          	= 0.0;  // ms
		float CONTROL_TI           	= 0.0;  // ms

		float SET_KP           		= 0.0;  // Arb U2
		float SET_TD           		= 0.0;  // ms
		float SET_TI           		= 0.0;  // ms

		// Input is the temperature or current
		float* LATEST_CONTROL;          // Arb U1
		float* LATEST_SET;				// Arb U2

		uint8_t ERROR               = 0;

	};

	enum class PID_STATE {
		SLEEP                = 0,
		CURRENT_MODE         = 1,
		TEMP_MODE            = 2,
		DISABLE              = 3,
		ENABLE               = 4
	};

	struct PID {
		PIDRegisters	REGISTERS;
		PID_STATE		State      = PID_STATE::SLEEP;
		uint16_t*		Ouput;

		bool 			SET_EN = true;

		float DeltaTime        	= 100.0; //ms

		uint16_t OutputMin     	= 0; // dac counts
		uint16_t OutputMax     	= 0; // dac counts

		// if _ is present it means it is not really meant to be written by an user
		// no, I am not using classes :p
		float 			_last_cont_err    = 0.0;  // [Arb U1]
		float 			_last_set_err		= 0.0;  // [Arb U2]
		float 			_cont_comm_err    = 0.0;  // [Arb U1*t]
		float 			_set_comm_err    	= 0.0;	// [Arb U2*t]
	};

	// Initializes the pid by setting all the internal held values to 0
	// and sets the Output to 0 (or def is set)
	// It TAKES the mutex and returns it to avoid race conditions
	void TCPID_init(PID& mod, const uint16_t& def = 0);
	
	// Restarts the internal PID values (commulative err and latest error)
	// and sets the Output to 0 (or def is set)
	// It does not take/return the mutex assosiated with registers.
	void TCPID_restart(PID& mod, const uint16_t& def = 0);

	// Runs the PID state machine. It can internally change its state
	// but the internal state can also be changed by changing TC_PID* state.
	void TCPID_update(PID&);
	
} // namepsace SBCQueens