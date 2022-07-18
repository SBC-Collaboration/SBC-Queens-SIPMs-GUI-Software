#include "SBCQueensTeensyHardware.h"

#include <arduino_freertos.h>
#include <Wire.h>

namespace SBCQueens {

	uint16_t N2_RELEASE_MAN_REG = 0;
	uint16_t N2_RELEASE_PID_REG = 0;
	Relay N2_RELEASE_RELAY;
    Relay N2_INPUT_RELAY;

	BME280_t LOCAL_BME280;
	BME280_t BOX_BME280;

	PeltierDriver PELTIER_DRIVER;

	MAX31865_t RTD_DAC_01;
	MAX31865_t RTD_DAC_02;

	PID PELTIER_PID;
	PID N2_PID;

	AnalogReadMV VACUUM_PRESSURE_SENSOR;
	AnalogReadMV NTWO_PRESSURE_SENSOR;

	void init_hardware_structs() {

		N2_RELEASE_RELAY.PIN 						= TWELVEV_PIN_TWO;
		N2_RELEASE_RELAY.Value 						= 0;

		N2_INPUT_RELAY.PIN 							= TWELVEV_PIN_THREE;
		N2_INPUT_RELAY.Value 						= 0;

		LOCAL_BME280.CS_PIN                         = BME_CS;
		LOCAL_BME280.ConfValue                      = BME_CONFIG_VALS::LOWEST_T_FILTER_MAX;
		LOCAL_BME280.ControlValue                   = BME_CONTROL_VALS::TEMP_ON_PRESS_ON_NORMAL;
		LOCAL_BME280.HumConfValue                   = BME_HUM_CONTROL_VALS::HUM_ON;

		BOX_BME280.CS_PIN                           = BOX_BME_CS;
		BOX_BME280.ConfValue                        = BME_CONFIG_VALS::LOWEST_T_FILTER_MAX;
		BOX_BME280.ControlValue                     = BME_CONTROL_VALS::TEMP_ON_PRESS_ON_NORMAL;
		BOX_BME280.HumConfValue                     = BME_HUM_CONTROL_VALS::HUM_ON;

		PELTIER_DRIVER.WireStream               	= &Wire;
		PELTIER_DRIVER.EN_PIN						= PELTIER_EN_PIN;
		PELTIER_DRIVER.MAX_CURRENT              	= 5.0;
		PELTIER_DRIVER.CurrentADCconfig         	= CURRENT_ADC_CONFIG_VALS::ADC_PID_OPTIMIZED_CONFIG;
		PELTIER_DRIVER.OutputDACconfig          	= OUTPUT_DAC_CONFIG_VALS::DAC_PID_OPTIMIZED_CONFIG;

		RTD_DAC_01.CS_PIN                           = RTD_ONE_CS;
		RTD_DAC_01.State                            = MAX31865_STATES::SLEEP;
		RTD_DAC_01.Configuration                    = MAX31865_CONF_VALS::PID_OPTIMIZED;

		RTD_DAC_02.CS_PIN                           = RTD_TWO_CS;
		RTD_DAC_02.State                            = MAX31865_STATES::SLEEP;
		RTD_DAC_02.Configuration                    = MAX31865_CONF_VALS::PID_OPTIMIZED;

		PELTIER_PID.State                        	= PID_STATE::SLEEP; // starts at sleep
		PELTIER_PID.SET_EN							= false;				// there is a control/set
		PELTIER_PID.OutputMax                   	= 0x0FFF; 			// 12 bits
		PELTIER_PID.OutputMin                   	= 0;				// 0 min
		PELTIER_PID.DeltaTime                   	= 100.0;			// ms
		PELTIER_PID.Ouput               			= &PELTIER_DRIVER.REGISTERS.LAST_DAC_REG;
		PELTIER_PID.REGISTERS.LATEST_CONTROL 		= &RTD_DAC_01.REGISTERS.LAST_TEMP_REG;
		PELTIER_PID.REGISTERS.LATEST_SET 			= &PELTIER_DRIVER.REGISTERS.LAST_CURRENT_REG;
		PELTIER_PID.REGISTERS.DESIRED_SET_VAL 		= 6.0;	// 2 amps max
		PELTIER_PID.REGISTERS.SET_KP 				= 200;	// pid values have been tested and proven
		PELTIER_PID.REGISTERS.SET_TI				= 100;
		PELTIER_PID.REGISTERS.SET_TD				= 0;

		N2_PID.State 								= PID_STATE::SLEEP;
		N2_PID.SET_EN 								= false;
		N2_PID.OutputMax                   			= 0x0001; 			// on/off pid
		N2_PID.OutputMin                   			= 0;			
		N2_PID.DeltaTime                   			= 100.0;			// ms
		N2_PID.Ouput 								= &N2_RELEASE_PID_REG;
		N2_PID.REGISTERS.LATEST_CONTROL				= &RTD_DAC_02.REGISTERS.LAST_TEMP_REG;

		VACUUM_PRESSURE_SENSOR.PIN 					= VACUUM_PIN;
		NTWO_PRESSURE_SENSOR.PIN 					= NTWO_PIN;

		relay_init(N2_RELEASE_RELAY);
		relay_init(N2_INPUT_RELAY);

		AnalogReadMV_init(VACUUM_PRESSURE_SENSOR);
		AnalogReadMV_init(NTWO_PRESSURE_SENSOR);
	
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