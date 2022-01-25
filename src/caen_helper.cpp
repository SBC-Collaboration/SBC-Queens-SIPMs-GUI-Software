#include "caen_helper.h"

#include <memory>
#include <spdlog/spdlog.h>

namespace SBCQueens {

	std::string translate_caen_code(const CAENComm_ErrorCode& err) noexcept {
		return "";
	}

	bool connect(CAEN_ptr& res, const CAEN_DGTZ_ConnectionType& ct,
		const int& ln, const int& cn, const uint32_t& addr) noexcept {

		if(res) {
			return false;
		}

		// Handle will move inside the CAEN_ptr if successful, 
		// otherwise, it will destroyed at the end of this function
		std::unique_ptr<int> handle = std::make_unique<int>(0);
		auto err = CAEN_DGTZ_OpenDigitizer(ct, ln, cn, addr, handle.get());
		
		if(err < 0) {
			spdlog::warn("Could not create CAEN resource: Error # {0}: ",
				static_cast<int>(err));
			return false;
		}

		spdlog::info("Created a new CAEN link handle: {0}", *handle);
		res = std::make_unique<CAEN>(ct, ln, cn, addr,
			handle, err);

		return true;
	}

	bool connect_usb(CAEN_ptr& res, const int& linkNum) noexcept {
		return connect(res, CAEN_DGTZ_ConnectionType::CAEN_DGTZ_USB,
			linkNum, 0, 0);
	}

	bool disconnect(CAEN_ptr& res) noexcept {

		if(!res){
			return false;
		}

		int& handle = *res->Handle;
		int err = CAEN_DGTZ_SWStopAcquisition(handle);
		err |= CAEN_DGTZ_ClearData(handle);
		err |= CAEN_DGTZ_CloseDigitizer(handle);

		if(err < 0) {
			res->LatestError = static_cast<CAEN_DGTZ_ErrorCode>(err);
			spdlog::warn("Could not disconnect CAEN resource, "
				"will delete internal resource anyways: Error # {0} ",
				static_cast<int>(err));
			return false;
		}

		res.release();
		return true;
	}

	bool reset(CAEN_ptr& res) noexcept {
		if(!res) {
			return false;
		}

		auto err = CAEN_DGTZ_Reset(*res->Handle);

		if(err < 0) {
			res->LatestError = err;
			spdlog::warn("Could not reset CAEN board.");
			return false;
		}

		return true;

	}

	bool calibrate(CAEN_ptr& res) noexcept {
		if(!res) {
			return false;
		}

		int& handle = *res->Handle;


		return true;
	}

	bool setup(CAEN_ptr& res,
		const std::initializer_list<ChannelConfig>& configs) noexcept {
		if(!res) {
			return false;
		}

		int& handle = *res->Handle;

		int err = 0;
		for(auto config : configs) {
			int channel_mask = 1 << config.Channel;

			err |= CAEN_DGTZ_SetRecordLength(handle, config.RecordLength);               
	    	err |= CAEN_DGTZ_SetChannelEnableMask(handle, channel_mask);                              

	    	// Trigger stuff
	    	err |= CAEN_DGTZ_SetChannelTriggerThreshold(handle,
	    		config.Channel,
	    		config.TriggerThreshold);
	    	err |= CAEN_DGTZ_SetChannelSelfTrigger(handle,
	    		config.TriggerMode,
	    		channel_mask);
	    	err |= CAEN_DGTZ_SetSWTriggerMode(handle,
	    		config.TriggerMode);
        	err |= CAEN_DGTZ_SetTriggerPolarity(handle,
        		config.Channel,
        		config.TriggerPolarity);

	    	err |= CAEN_DGTZ_SetMaxNumEventsBLT(handle,
	    		config.MaxEventsPerRead);
        	err |= CAEN_DGTZ_SetAcquisitionMode(handle,
        		config.AcqMode);
        	err |= CAEN_DGTZ_SetChannelDCOffset(handle,
        		config.Channel,
        		config.DCOffset);


        	// Writes to the registers that holds the DC range
        	err |= write_register(res, 0x1028 | (config.Channel & 0x0F) << 8,
        		config.DCRange & 0x0001);

        	// Board config register
      //   	err |= write_register(res, 0x8000,
      //   		0x0010 |
      //   		((config.TriggerOverlappingEn & 0x0001) << 1) |
      //   		((config.TriggerPolarity & 0x0001) << 6)
    		// );
		}

		if(err < 0) {
			res->LatestError = static_cast<CAEN_DGTZ_ErrorCode>(err);
			spdlog::warn("There was an error during setup.");
			return false;
		}

		return true;
	}

	bool enable_acquisition(CAEN_ptr& res) noexcept {
		if(!res) {
			return false;
		}

		int& handle = *res->Handle;

		int err = CAEN_DGTZ_MallocReadoutBuffer(handle,
			&res->Data.Buffer, &res->Data.TotalSizeBuffer);

		err |= CAEN_DGTZ_MallocReadoutBuffer(handle,
			&res->BackupData.Buffer, &res->BackupData.TotalSizeBuffer);

		err |= CAEN_DGTZ_ClearData(handle);
		err |= CAEN_DGTZ_SWStartAcquisition(handle);
		

		if(err < 0) {
			res->LatestError = static_cast<CAEN_DGTZ_ErrorCode>(err);
			spdlog::warn("Could not enable acquisition.");
			return false;
		}

		spdlog::info("Allocated memory with a total size of {0}",
			res->Data.TotalSizeBuffer);

		return true;
	}

	bool disable_acquisition(CAEN_ptr& res) noexcept {
		if(!res) {
			return false;
		}

		auto err = CAEN_DGTZ_SWStopAcquisition(*res->Handle);

		if(err < 0) {
			res->LatestError = err;
			spdlog::warn("Could not disable acquisition.");
			return false;
		}

		return true;
	}

	bool write_register(CAEN_ptr& res, uint32_t&& addr,
		const uint32_t& value) noexcept {

		if(!res) {
			return false;
		}

		auto err = CAEN_DGTZ_WriteRegister(*res->Handle, addr, value);
		if(err < 0) {
			res->LatestError = err;
			spdlog::warn("Could read register with address: {0}.", addr);
			return false;
		}

		return true;
	}

	bool read_register(CAEN_ptr& res, uint32_t&& addr,
		uint32_t& value) noexcept {

		if(!res) {
			return false;
		}

		auto err = CAEN_DGTZ_ReadRegister(*res->Handle, addr, &value);
		if(err < 0) {
			res->LatestError = err;
			spdlog::warn("Could read register with address: {0}.", addr);
			return false;
		}

		return true;
	}

	bool software_trigger(CAEN_ptr& res) noexcept {

		if(!res) {
			return false;
		}

		auto err = CAEN_DGTZ_SendSWtrigger(*res->Handle);

		if(err < 0) {
			res->LatestError = err;
			spdlog::warn("Failed to send a software trigger.");
			return false;
		}

		return true;

	}

	void retrieve_data(CAEN_ptr& res) noexcept {
		int& handle = *res->Handle;

		int err = CAEN_DGTZ_ReadData(handle,
			CAEN_DGTZ_ReadMode_t::CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,
			res->WriteData->Buffer,
			&res->WriteData->DataSize);

		err |= CAEN_DGTZ_GetNumEvents(handle,
			res->WriteData->Buffer,
			res->WriteData->DataSize,
			&res->WriteData->NumEvents);

		// After this, swap ReadData and WriteData
		// So the recently wrote to data becomes the to read data.
		// and the read data becomes the next to write data
		res->WriteData.swap(res->ReadData);

	   	res->LatestError = static_cast<CAEN_DGTZ_ErrorCode>(err);
	}

	Event extract_event(CAEN_ptr& res, const uint32_t& i) noexcept {
		int& handle = *res->Handle;

		if(i >= res->Data.NumEvents) {
			return Event();
		}

		Event newEvent;
	    int err = CAEN_DGTZ_GetEventInfo(handle,
		    	res->ReadData->Buffer,
		    	res->ReadData->DataSize,
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
    	res->ReadData->Buffer,
    	res->ReadData->DataSize,
    	i,
    	&evt.Info,
    	&evt.DataPtr);

	    CAEN_DGTZ_DecodeEvent(handle,
    	evt.DataPtr,
    	reinterpret_cast<void**>(&evt.Data));
	}

	void allocate_event(CAEN_ptr& res, Event& evt) noexcept {
		res->LatestError = CAEN_DGTZ_AllocateEvent(*res->Handle,
			reinterpret_cast<void**>(&evt.Data));
	}

	void free_event(CAEN_ptr& res, Event& evt) noexcept {
		res->LatestError = CAEN_DGTZ_FreeEvent(*res->Handle.get(),
			reinterpret_cast<void**>(&evt.Data) );
	}

	void clear_data(CAEN_ptr& res) noexcept {

		int& handle = *res->Handle;
		int err = CAEN_DGTZ_SWStopAcquisition(handle);
		err |= CAEN_DGTZ_ClearData(handle);
		err |= CAEN_DGTZ_SWStartAcquisition(handle);
		res->LatestError = static_cast<CAEN_DGTZ_ErrorCode>(err);

	}

	bool check_for_error(CAEN_ptr& res) noexcept {

		if(!res) {
			return false;
		}

		auto err = res->LatestError;

		if(err < 0) {
			spdlog::warn("Latest error code: {0}", err);
			return false;
		}

		return true;
	}

} // namespace SBCQueens