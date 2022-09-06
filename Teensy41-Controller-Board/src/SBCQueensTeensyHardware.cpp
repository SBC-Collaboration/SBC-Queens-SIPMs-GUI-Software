#include "SBCQueensTeensyHardware.h"

#include <arduino_freertos.h>
#include <Wire.h>

namespace SBCQueens {

	BME280_t LOCAL_BME280;

	PeltierDriver PELTIER_DRIVER;

#ifdef NEW_RTD_BOARD
	RTDBoard<NUM_RTD_PER_BOARD> RTD_BOARDS[NUM_RTD_BOARDS];
#else 
	MAX31865_t RTD_BOARDS[NUM_RTD_BOARDS];
#endif

	PID PELTIER_PID;

	AnalogReadMV VACUUM_PRESSURE_SENSOR;

	void init_hardware_structs() {


		LOCAL_BME280.CS_PIN                         = BME_CS;
		LOCAL_BME280.ConfValue                      = BME_CONFIG_VALS::LOWEST_T_FILTER_MAX;
		LOCAL_BME280.ControlValue                   = BME_CONTROL_VALS::TEMP_ON_PRESS_ON_NORMAL;
		LOCAL_BME280.HumConfValue                   = BME_HUM_CONTROL_VALS::HUM_ON;

		// BOX_BME280.CS_PIN                           = BOX_BME_CS;
		// BOX_BME280.ConfValue                        = BME_CONFIG_VALS::LOWEST_T_FILTER_MAX;
		// BOX_BME280.ControlValue                     = BME_CONTROL_VALS::TEMP_ON_PRESS_ON_NORMAL;
		// BOX_BME280.HumConfValue                     = BME_HUM_CONTROL_VALS::HUM_ON;

		PELTIER_DRIVER.WireStream               	= &Wire;
		PELTIER_DRIVER.EN_PIN						= PELTIER_EN_PIN;
		PELTIER_DRIVER.MAX_CURRENT              	= 5.0;
		PELTIER_DRIVER.CurrentADCconfig         	= CURRENT_ADC_CONFIG_VALS::ADC_PID_OPTIMIZED_CONFIG;
		PELTIER_DRIVER.OutputDACconfig          	= OUTPUT_DAC_CONFIG_VALS::DAC_PID_OPTIMIZED_CONFIG;

#ifdef NEW_RTD_BOARD
		RTD_BOARDS[0].MCP23S08_CS 						= 20;
		RTD_BOARDS[0].ADC_CS[0]							= 23;
		RTD_BOARDS[0].ADC_CS[1]							= 22;
		RTD_BOARDS[0].ADC_CS[2]							= 21;
#else
		RTD_BOARDS[0].CS_PIN                           = RTD_ONE_CS;
		RTD_BOARDS[0].State                            = MAX31865_STATES::SLEEP;
		RTD_BOARDS[0].Configuration                    = MAX31865_CONF_VALS::PID_OPTIMIZED;

		RTD_BOARDS[1].CS_PIN                           = RTD_TWO_CS;
		RTD_BOARDS[1].State                            = MAX31865_STATES::SLEEP;
		RTD_BOARDS[1].Configuration                    = MAX31865_CONF_VALS::PID_OPTIMIZED;
#endif

		PELTIER_PID.State                        	= PID_STATE::SLEEP; // starts at sleep
		PELTIER_PID.SET_EN							= true;				// there is a control/set
		PELTIER_PID.OutputMax                   	= 0x0FFF; 			// 12 bits
		PELTIER_PID.OutputMax                   	= 3400; 			// 5 amps approx
		PELTIER_PID.OutputMin                   	= 0;				// 0 min
		PELTIER_PID.DeltaTime                   	= 100.0;			// ms
		PELTIER_PID.Ouput               			= &PELTIER_DRIVER.REGISTERS.LAST_DAC_REG;
#ifdef NEW_RTD_BOARD
		PELTIER_PID.REGISTERS.LATEST_CONTROL 		= &RTD_BOARDS[0].LAST_TEMP_VAL[0];
#else
		PELTIER_PID.REGISTERS.LATEST_CONTROL 		= &RTD_BOARDS[0].REGISTERS.LAST_TEMP_REG;
#endif
		PELTIER_PID.REGISTERS.LATEST_SET 			= &PELTIER_DRIVER.REGISTERS.LAST_CURRENT_REG;
		PELTIER_PID.REGISTERS.DESIRED_SET_VAL 		= 5.0;	// amps max
		PELTIER_PID.REGISTERS.SET_KP 				= 100;	// pid values have been tested and proven
		PELTIER_PID.REGISTERS.SET_TI				= 200;
		PELTIER_PID.REGISTERS.SET_TD				= 0;

		// N2_PID.State 								= PID_STATE::SLEEP;
		// N2_PID.SET_EN 								= false;
		// N2_PID.OutputMax                   			= 0x0001; 			// on/off pid
		// N2_PID.OutputMin                   			= 0;			
		// N2_PID.DeltaTime                   			= 100.0;			// ms
		// N2_PID.Ouput 								= &N2_RELEASE_PID_REG;
		// N2_PID.REGISTERS.LATEST_CONTROL				= &RTD_DAC_02.REGISTERS.LAST_TEMP_REG;

		VACUUM_PRESSURE_SENSOR.PIN 					= VACUUM_PIN;
		// NTWO_PRESSURE_SENSOR.PIN 					= NTWO_PIN;

		// relay_init(N2_RELEASE_RELAY);
		// relay_init(N2_INPUT_RELAY);

		AnalogReadMV_init(VACUUM_PRESSURE_SENSOR);
		// AnalogReadMV_init(NTWO_PRESSURE_SENSOR);

#ifdef NEW_RTD_BOARD
		for ( uint8_t i = 0; i < NUM_RTD_BOARDS; i++) {
			RTDBoard_init(RTD_BOARDS[i]);
		}
#endif
	
	}

	void init_pins() {
	/// Pin Initializations
		pinMode(        TWELVEV_PIN_ONE,    arduino::OUTPUT);
		digitalWrite(   TWELVEV_PIN_ONE,    arduino::LOW);

		pinMode(        TWELVEV_PIN_TWO,    arduino::OUTPUT);
		digitalWrite(   TWELVEV_PIN_TWO,    arduino::LOW);

		pinMode(        TWELVEV_PIN_THREE,  arduino::OUTPUT);
		digitalWrite(   TWELVEV_PIN_THREE,  arduino::LOW);

		// SPI CS Pins
		pinMode(        RTD_ONE_CS,         arduino::OUTPUT);
		digitalWrite(   RTD_ONE_CS,         arduino::HIGH);

		pinMode(        RTD_TWO_CS,         arduino::OUTPUT);
		digitalWrite(   RTD_TWO_CS,         arduino::HIGH);

		pinMode(        BME_CS,             arduino::OUTPUT);
		digitalWrite(   BME_CS,             arduino::HIGH);

		pinMode(        BOX_BME_CS,         arduino::OUTPUT);
		digitalWrite(   BOX_BME_CS,         arduino::HIGH);
		/// !Pin Initializations
	}

} // namespace SBCQueens