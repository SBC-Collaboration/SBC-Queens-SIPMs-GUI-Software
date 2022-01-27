#include "caen_helper.h"

#include <memory>

namespace SBCQueens {

	std::string translate_caen_code(const CAENComm_ErrorCode& err) noexcept {
		return "";
	}

	void connect(CAEN_ptr& res, const CAEN_DGTZ_ConnectionType& ct,
		const int& ln, const int& cn, const uint32_t& addr) noexcept {

		CAENError err;
		// If the item exists, pass creation
		if(res) {
			err = CAENError {
				.ErrorMessage = "Resource already connected",
				.ErrorCode = CAEN_DGTZ_ErrorCode::CAEN_DGTZ_GenericError,
				.isError = true
			};
		}

		// Handle will move inside the CAEN_ptr if successful, 
		// otherwise, it will destroyed at the end of this function
		std::unique_ptr<int> handle = std::make_unique<int>(0);
		auto errCode = CAEN_DGTZ_OpenDigitizer(ct, ln, cn, addr, handle.get());
		
		if(errCode < 0) {
			err = CAENError {
				.ErrorMessage = "Error while attempting connection",
				.ErrorCode = errCode,
				.isError = true
			};
		}

		res = std::make_unique<CAEN>(ct, ln, cn, addr,
			handle, err);

	}

	void connect_usb(CAEN_ptr& res, const int& linkNum) noexcept {
		connect(res, CAEN_DGTZ_ConnectionType::CAEN_DGTZ_USB,
			linkNum, 0, 0);
	}

	void disconnect(CAEN_ptr& res) noexcept {

		if(!res){
			return;
		}

		if(res->LatestError.isError) {
			return;
		}

		int& handle = *res->Handle;
		// Before disconnecting, stop acquisition, and clear data
		// (clear data should only be called after the acquisition has been
		// stopped)
		// Resources are freed when the pointer is released
		int errCode = CAEN_DGTZ_SWStopAcquisition(handle);
		errCode |= CAEN_DGTZ_ClearData(handle);
		errCode |= CAEN_DGTZ_CloseDigitizer(handle);

		if(errCode < 0) {
			res->LatestError = CAENError {
				.ErrorMessage = "Could not disconnect CAEN resource. "
				"Will delete internal resource anyways.",
				.ErrorCode = static_cast<CAEN_DGTZ_ErrorCode>(errCode),
				.isError = true
			};
		}

		res.release();
	}

	void reset(CAEN_ptr& res) noexcept {
		if(!res) {
			return;
		}

		if(res->LatestError.isError) {
			return;
		}

		auto err = CAEN_DGTZ_Reset(*res->Handle);

		if(err < 0) {
			res->LatestError = CAENError {
				.ErrorMessage = "Could not reset CAEN board.",
				.ErrorCode = err,
				.isError = true
			};
		}

	}

	void calibrate(CAEN_ptr& res) noexcept {
		if(!res) {
			res->LatestError = CAENError {
				.ErrorMessage = "Resource does not exist, "
				"nothing to calibrate.",
				.ErrorCode = CAEN_DGTZ_ErrorCode::CAEN_DGTZ_GenericError,
				.isError = true
			};
		}

		if(res->LatestError.isError) {
			return;
		}

		int& handle = *res->Handle;


	}

	void setup(CAEN_ptr& res,
		const std::initializer_list<ChannelConfig>& configs) noexcept {

		if(!res) {
			return;
		}

		if(res->LatestError.isError) {
			return;
		}

		int& handle = *res->Handle;

		int err = 0, total_err = 0;
		std::string err_msg = "";
		for(auto config : configs) {
			int channel_mask = 1 << config.Channel;

			err = CAEN_DGTZ_SetRecordLength(handle, config.RecordLength);
			if(err < 0) {
				total_err |= err;
				err_msg += "CAEN_DGTZ_SetRecordLength Failed. ";
			}

	    	err = CAEN_DGTZ_SetChannelEnableMask(handle, channel_mask);
			if(err < 0) {
				total_err |= err;
				err_msg += "CAEN_DGTZ_SetChannelEnableMask Failed. ";
			}

	    	err = CAEN_DGTZ_SetMaxNumEventsBLT(handle,
	    		config.MaxEventsPerRead);
			if(err < 0) {
				total_err |= err;
				err_msg += "CAEN_DGTZ_SetMaxNumEventsBLT Failed. ";
			}

	    	// Trigger stuff
	    	err = CAEN_DGTZ_SetChannelTriggerThreshold(handle,
	    		config.Channel,
	    		config.TriggerThreshold);
			if(err < 0) {
				total_err |= err;
				err_msg += "CAEN_DGTZ_SetChannelTriggerThreshold Failed. ";
			}

	    	err = CAEN_DGTZ_SetChannelSelfTrigger(handle,
	    		config.TriggerMode,
	    		channel_mask);
			if(err < 0) {
				total_err |= err;
				err_msg += "CAEN_DGTZ_SetChannelSelfTrigger Failed. ";
			}

	    	err = CAEN_DGTZ_SetSWTriggerMode(handle,
	    		config.TriggerMode);
			if(err < 0) {
				total_err |= err;
				err_msg += "CAEN_DGTZ_SetSWTriggerMode Failed. ";
			}


	    	// Acquisition mode comes in four flavors:
	    	// CAEN_DGTZ_SW_CONTROLLED
	    	// 		Start/stop of the runt akes place on software
	    	// 		command (by calling CAEN_DGTZ_SWStartAcquisition )
	    	// CAEN_DGTZ_FIRST_TRG_CONTROLLED
	    	// 		Run starts on the first trigger pulse (rising edge on TRG-IN)
	    	// 		actual triggers start from the second pulse from there
	    	// CAEN_DGTZ_S_IN_CONTROLLED
	    	//		S-IN/GPI controller (depends on the board). Acquisition
	    	// 		starts on edge of the GPI/S-IN
	    	// CAEN_DGTZ_LVDS_CONTROLLED
	    	//		VME ONLY, like S_IN_CONTROLLER but uses LVDS.
        	err = CAEN_DGTZ_SetAcquisitionMode(handle,
        		config.AcqMode);
			if(err < 0) {
				total_err |= err;
				err_msg += "CAEN_DGTZ_SetAcquisitionMode Failed. ";
			}

        	err = CAEN_DGTZ_SetChannelDCOffset(handle,
        		config.Channel,
        		config.DCOffset);
			if(err < 0) {
				total_err |= err;
				err_msg += "CAEN_DGTZ_SetChannelDCOffset Failed. ";
			}

        	err = CAEN_DGTZ_SetPostTriggerSize(handle,
        		config.PostTriggerPorcentage);
			if(err < 0) {
				total_err |= err;
				err_msg += "CAEN_DGTZ_SetPostTriggerSize Failed. ";
			}

        	// Writes to the registers that holds the DC range
        	// For 5730 it is the register 0x1n28
			write_register(res, 0x1028 | (config.Channel & 0x0F) << 8,
        		config.DCRange & 0x0001);

			err = res->LatestError.ErrorCode;
			if(err < 0) {
				total_err |= err;
				err_msg += "write_register to reg 0x1n28 Failed. ";
			}

        	//TODO(Hector): add pre-buffer %
        	// and polling/trigger config

        	// Board config register
        	// 0 = Trigger overlapping not allowed
        	// 1 = trigger overlapping allowed
        	write_register(res, 0x8000,
        		0x0010 |
        		((config.TriggerOverlappingEn & 0x0001) << 1)
    		);
    		err = res->LatestError.ErrorCode;
			if(err < 0) {
				total_err |= err;
				err_msg += "write_register to reg 0x8000 Failed. ";
			}

        	err = CAEN_DGTZ_SetTriggerPolarity(handle,
        		config.Channel,
        		config.TriggerPolarity);
			if(err < 0) {
				total_err |= err;
				err_msg += "CAEN_DGTZ_SetTriggerPolarity Failed. ";
			}

		}

		if(err < 0) {
			res->LatestError = CAENError {
				.ErrorMessage = "There was en error during setup!" + err_msg,
				.ErrorCode = static_cast<CAEN_DGTZ_ErrorCode>(total_err),
				.isError = true
			};
		}

	}

	void enable_acquisition(CAEN_ptr& res) noexcept {
		if(!res) {
			return;
		}

		if(res->LatestError.isError) {
			return;
		}

		int& handle = *res->Handle;

		int err = CAEN_DGTZ_MallocReadoutBuffer(handle,
			&res->Data.Buffer, &res->Data.TotalSizeBuffer);

		err |= CAEN_DGTZ_ClearData(handle);
		err |= CAEN_DGTZ_SWStartAcquisition(handle);

		if(err < 0) {
			res->LatestError = CAENError {
				.ErrorMessage = "There was en error while trying to enable"
				"acquisition or allocate memory for buffers.",
				.ErrorCode = static_cast<CAEN_DGTZ_ErrorCode>(err),
				.isError = true
			};
		}
	}

	void disable_acquisition(CAEN_ptr& res) noexcept {
		if(!res) {
			return;
		}

		if(res->LatestError.isError) {
			return;
		}

		auto err = CAEN_DGTZ_SWStopAcquisition(*res->Handle);

		if(err < 0) {
			res->LatestError = CAENError {
				.ErrorMessage = "There was en error while trying to disable"
				"acquisition.",
				.ErrorCode = err,
				.isError = true
			};
		}
	}

	void write_register(CAEN_ptr& res, uint32_t&& addr,
		const uint32_t& value) noexcept {

		if(!res) {
			return;
		}

		if(res->LatestError.isError) {
			return;
		}

		auto err = CAEN_DGTZ_WriteRegister(*res->Handle, addr, value);
		if(err < 0) {
			res->LatestError = CAENError {
				.ErrorMessage = "There was en error while trying to write"
				"to register.",
				.ErrorCode = err,
				.isError = true
			};
		}
	}

	void read_register(CAEN_ptr& res, uint32_t&& addr,
		uint32_t& value) noexcept {

		if(!res) {
			return;
		}

		if(res->LatestError.isError) {
			return;
		}

		auto err = CAEN_DGTZ_ReadRegister(*res->Handle, addr, &value);
		if(err < 0) {
			res->LatestError = CAENError {
				.ErrorMessage = "There was en error while trying to read"
				"to register.",
				.ErrorCode = err,
				.isError = true
			};
		}
	}

	void software_trigger(CAEN_ptr& res) noexcept {

		if(!res) {
			return;
		}

		if(res->LatestError.isError) {
			return;
		}

		auto err = CAEN_DGTZ_SendSWtrigger(*res->Handle);

		if(err < 0) {
			res->LatestError = CAENError {
				.ErrorMessage = "Failed to send a software trigger",
				.ErrorCode = err,
				.isError = true
			};
		}
	}

	uint32_t get_events_in_buffer(CAEN_ptr& res) noexcept {
		uint32_t events = 0;
		// For 5730 it is the register 0x814C or 0x812C?
		read_register(res, 0x812C, events);

		return events;
	}

	void retrieve_data(CAEN_ptr& res) noexcept {
		int& handle = *res->Handle;

		int err = CAEN_DGTZ_ReadData(handle,
			CAEN_DGTZ_ReadMode_t::CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,
			res->Data.Buffer,
			&res->Data.DataSize);

		err |= CAEN_DGTZ_GetNumEvents(handle,
			res->Data.Buffer,
			res->Data.DataSize,
			&res->Data.NumEvents);

	   	res->LatestError.ErrorCode = static_cast<CAEN_DGTZ_ErrorCode>(err);
	}

	Event extract_event(CAEN_ptr& res, const uint32_t& i) noexcept {
		int& handle = *res->Handle;

		if(i >= res->Data.NumEvents) {
			return Event();
		}

		Event newEvent;
	    int err = CAEN_DGTZ_GetEventInfo(handle,
		    	res->Data.Buffer,
		    	res->Data.DataSize,
		    	i,
		    	&newEvent.Info,
		    	&newEvent.DataPtr);

	    if(err < 0) {
	    	return Event();
	    }

	    err |= CAEN_DGTZ_DecodeEvent(handle,
		    	newEvent.DataPtr,
		    	reinterpret_cast<void**>(&newEvent.Data));

	    return err < 0 ? Event() : newEvent;

	}

	void extract_event(CAEN_ptr& res, const uint32_t& i, Event& evt) noexcept {
		int& handle = *res->Handle;

	    CAEN_DGTZ_GetEventInfo(handle,
	    	res->Data.Buffer,
	    	res->Data.DataSize,
	    	i,
	    	&evt.Info,
	    	&evt.DataPtr);

	    CAEN_DGTZ_DecodeEvent(handle,
    		evt.DataPtr,
    		reinterpret_cast<void**>(&evt.Data));
	}

	void allocate_event(CAEN_ptr& res, Event& evt) noexcept {
		res->LatestError.ErrorCode = CAEN_DGTZ_AllocateEvent(*res->Handle,
			reinterpret_cast<void**>(&evt.Data));
		res->LatestError.isError = res->LatestError.ErrorCode < 0;
	}

	void free_event(CAEN_ptr& res, Event& evt) noexcept {
		if(!res) {
			return;
		}

		if(res->LatestError.isError) {
			return;
		}

		res->LatestError.ErrorCode = CAEN_DGTZ_FreeEvent(*res->Handle.get(),
			reinterpret_cast<void**>(&evt.Data) );
		res->LatestError.isError = res->LatestError.ErrorCode < 0;
	}

	void clear_data(CAEN_ptr& res) noexcept {

		if(!res) {
			return;
		}

		if(res->LatestError.isError) {
			return;
		}

		int& handle = *res->Handle;
		int err = CAEN_DGTZ_SWStopAcquisition(handle);
		err |= CAEN_DGTZ_ClearData(handle);
		err |= CAEN_DGTZ_SWStartAcquisition(handle);
		if(err < 0) {
			res->LatestError = CAENError {
				.ErrorMessage = "Failed to send a software trigger",
				.ErrorCode = static_cast<CAEN_DGTZ_ErrorCode>(err),
				.isError = true
			};
		}

	}

	// uint32_t channel_self_trigger_meter(CAEN_ptr& res, uint8_t channel) noexcept {
	// 	uint32_t freq = 0;
	// 	// For 5730 it is 0x1nEC where n is the channel
	// 	read_register(res, 0x10EC | (channel & 0x0F) << 8,
 //        		freq);

	// 	return freq;
	// }

} // namespace SBCQueens