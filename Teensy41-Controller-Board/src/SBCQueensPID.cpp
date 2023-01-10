#include "SBCQueensPID.h"

#include <limits>

#include "SBCQueensTeensyBasics.h"

namespace SBCQueens {

	void TCPID_init(PID& controller, const uint16_t& def) {
		take_reg_mux();
		// Reset all the values
			controller._cont_comm_err   = 0.0;
			controller._set_comm_err    = 0.0;
			controller._last_set_err    = 0.0;
			controller._last_cont_err   = 0.0;

			*controller.Ouput = def;

			controller.State = PID_STATE::SLEEP;
		give_reg_mux();
	}

	void TCPID_restart(PID& controller, const uint16_t& def) {
		
		controller._cont_comm_err   = 0.0;
		controller._set_comm_err    = 0.0;
		controller._last_set_err    = 0.0;
		controller._last_cont_err   = 0.0;

		*controller.Ouput = def;
	}

	void TCPID_update(PID& controller) {

		// References to make the new lines easier to read without impacting perfomance
		// (as I expect these lines to be optimized during compile time)
		float& delta_time           = controller.DeltaTime;
		float& current_setpoint     = controller.REGISTERS.DESIRED_SET_VAL;
		float& temperature_setpoint = controller.REGISTERS.DESIRED_CONTROL_VAL;
		float& latest_current_meas  = *controller.REGISTERS.LATEST_SET;
		float& latest_temp_meas     = *controller.REGISTERS.LATEST_CONTROL;
		
		switch(controller.State) {
			case PID_STATE::CURRENT_MODE:
			{
				// This state is forbidden if SET_EN is false
				if(!controller.SET_EN) {
					take_reg_mux();
						controller.State = PID_STATE::TEMP_MODE;
					give_reg_mux();
					break;
				}

				float& latest_current_error = controller._last_set_err;

				float& kp = controller.REGISTERS.SET_KP;
				float& ti = controller.REGISTERS.SET_TI;
				float& td = controller.REGISTERS.SET_TD;

				float pid_error = current_setpoint - latest_current_meas;
				float pid_comm_err = controller._set_comm_err;
				// Integral term
				if(ti != 0) {
					pid_comm_err += pid_error*(delta_time / ti);

					// Values higher than 32767 do not make sense as this is a 16 bit PID
					// To give headroom for when calculating the output, we use 16384
					// or 2^14
					float cmp = kp*pid_comm_err;
					if (cmp > 49152.0f) {
						pid_comm_err = 49152.0f / kp;
					} else if (cmp < -49152.0f) {
						pid_comm_err = -49152.0f / kp;
					}
				}

				// Derivative term
				// Td*de/DELTA_T
				float derivative_error = 0.0;
				if(td != 0) {
					derivative_error = (pid_error - latest_current_error)*(td*delta_time);
				}

				// Before updating the register, ceil the tmp value to max and min values
				uint16_t tmp = static_cast<uint16_t>(kp*(pid_error + pid_comm_err + derivative_error));
				if(tmp > controller.OutputMax) {
					tmp = controller.OutputMax;
				} else if(tmp < controller.OutputMin) {
					tmp = controller.OutputMin;
				}

				take_reg_mux();
					// If the difference in temperatures is less than 5.0 change to temp mode
					if(abs(temperature_setpoint - latest_temp_meas) < 5.0) {
						// if ( latest_current_meas < 0.95*controller.REGISTERS.DESIRED_SET_VAL ) {
							controller.State = PID_STATE::TEMP_MODE;
							controller._cont_comm_err = 0.0;
						// }
					}

					*controller.Ouput = tmp;
					controller._set_comm_err = pid_comm_err;
					controller._last_set_err = pid_error;
				give_reg_mux();
				break;
			}
				
			case PID_STATE::TEMP_MODE:
			{
				float& latest_temp_error  = controller._last_cont_err;

				float& kp = controller.REGISTERS.CONTROL_KP;
				float& ti = controller.REGISTERS.CONTROL_TI;
				float& td = controller.REGISTERS.CONTROL_TD;

				float pid_error = temperature_setpoint - latest_temp_meas;
				// Temp solution. It is not the best but it will mitigate overflow
				// TODO(Hector): investigate how to make this faster and better
				float pid_comm_err = controller._cont_comm_err;
				// Integral term
				if(ti != 0) {
					pid_comm_err += pid_error*(delta_time / ti);

					// Values higher than 32767 do not make sense as this is a 16 bit PID
					// To give headroom for when calculating the output, we use 16384
					// or 2^14
					float cmp = kp*pid_comm_err;
					if (cmp > 49152.0f) {
						pid_comm_err = 49152.0f / kp;
					} else if (cmp < -49152.0f) {
						pid_comm_err = -49152.0f / kp;
					}
				}

				// Derivative term
				// Td*de/DELTA_T
				float derivative_error = 0.0;
				if(td != 0) {
					derivative_error = td*(pid_error - latest_temp_error)/delta_time;
				}

				// Before updating the register, ceil it to max and min values
				uint16_t tmp = static_cast<uint16_t>(kp*(pid_error + pid_comm_err + derivative_error));
				if(tmp > controller.OutputMax) {
					tmp = controller.OutputMax;
				} else if(tmp < controller.OutputMin) {
					tmp = controller.OutputMin;
				}

				// If the difference in temperatures is higher or equal than 5.0 change to current mode
				
				take_reg_mux();
					// Changes states only if enabled and away from 5 degC
					if(abs(pid_error) >= 5.0 && controller.SET_EN) {
						controller.State = PID_STATE::CURRENT_MODE;
						controller._set_comm_err = 0.0;
					}

					*controller.Ouput = tmp;
					controller._cont_comm_err = pid_comm_err;
					controller._last_cont_err = pid_error;
				give_reg_mux();
				break;
			}

			case PID_STATE::DISABLE:
			{
				take_reg_mux();
					TCPID_restart(controller);
					*controller.Ouput = 0;
				// peltierdriver_disable(*controller.PeltierDriver);
					controller.State = PID_STATE::SLEEP;
				give_reg_mux();
				
				break;
			}


			case PID_STATE::ENABLE:
			{
				take_reg_mux();
					TCPID_restart(controller);
				// peltierdriver_enable(*controller.PeltierDriver);
					controller.State = PID_STATE::CURRENT_MODE;
				give_reg_mux();

				break;
			}
				
			case PID_STATE::SLEEP:
			default:
				// Nothing happens
				break;
		}

	}
} // SBCQueens namespace