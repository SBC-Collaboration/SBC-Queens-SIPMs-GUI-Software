#include "caen_helper.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace SBCQueens {

	std::string translate_caen_code(const CAENComm_ErrorCode& err) noexcept {
		return "";
	}

	CAENError connect(CAEN& res, const CAENDigitizerModel& model,
		const CAEN_DGTZ_ConnectionType& ct,
		const int& ln, const int& cn, const uint32_t& addr) noexcept {

		CAENError err;
		// If the item exists, pass on the creation
		if(res) {
			err = CAENError {
				.ErrorMessage = "Resource already connected",
				.ErrorCode = CAEN_DGTZ_ErrorCode::CAEN_DGTZ_GenericError,
				.isError = true
			};

			return err;
		}

		int handle = 0;
		auto errCode = CAEN_DGTZ_OpenDigitizer(ct, ln, cn, addr, &handle);
		
		if(errCode >= 0) {
			res = std::make_unique<caen>(model, ct, ln, cn, addr,
				handle, err);
		} else {
			return CAENError {
				.ErrorMessage = "Failed to connect to the resource",
				.ErrorCode = errCode,
				.isError = true
			};
		}

		return CAENError();

	}

	CAENError connect_usb(CAEN& res, const CAENDigitizerModel& model,
		const int& linkNum) noexcept {
		return connect(res, model, CAEN_DGTZ_ConnectionType::CAEN_DGTZ_USB,
			linkNum, 0, 0);
	}

	CAENError disconnect(CAEN& res) noexcept {

		if(!res){
			return CAENError();
		}

		int& handle = res->Handle;
		// Before disconnecting, stop acquisition, and clear data
		// (clear data should only be called after the acquisition has been
		// stopped)
		// Resources are freed when the pointer is released
		int errCode = CAEN_DGTZ_SWStopAcquisition(handle);
		errCode |= CAEN_DGTZ_ClearData(handle);
		errCode |= CAEN_DGTZ_CloseDigitizer(handle);

		if(errCode < 0) {
			return CAENError {
				.ErrorMessage = "Error while disconnecting CAEN resource. "
				"Will delete internal resource anyways.",
				.ErrorCode = static_cast<CAEN_DGTZ_ErrorCode>(errCode),
				.isError = true
			};
		}

		res.release();
		return CAENError();
	}

	void reset(CAEN& res) noexcept {
		if(!res) {
			return;
		}

		auto err = CAEN_DGTZ_Reset(res->Handle);

		if(err < 0) {
			res->LatestError = CAENError {
				.ErrorMessage = "Could not reset CAEN board.",
				.ErrorCode = err,
				.isError = true
			};
		}

	}

	void calibrate(CAEN& res) noexcept {
		if(!res) {
			return;
		}

		if(res->LatestError.isError) {
			return;
		}

		//int& handle = res->Handle;

	}

	void setup(CAEN& res, CAENGlobalConfig g_config,
		std::initializer_list<CAENChannelConfig> ch_configs) noexcept {

		if(!res) {
			return;
		}

		if(res->LatestError.isError) {
			return;
		}

		int& handle = res->Handle;
		int latest_err = 0;
		std::string err_msg = "";

		// This lambda wraps whatever CAEN function you pass to it,
		// checks the error, and if there was an error, add the error msg to it
		// If there is an error, it also does not execute the function.
		auto error_wrap = [&](std::string msg, auto f, auto... args) {
			if(latest_err >= 0) {
				auto err = f(args...);
				if(err < 0) {
					latest_err = err;
					err_msg += msg;
				}
			}
		};

		// Global config
		res->GlobalConfig = g_config;
		error_wrap("CAEN_DGTZ_SetMaxNumEventsBLT Failed. ",
			CAEN_DGTZ_SetMaxNumEventsBLT,
			handle, res->GlobalConfig.MaxEventsPerRead);
		// Before:
     	//err = CAEN_DGTZ_SetMaxNumEventsBLT(handle,
     	//	res->GlobalConfig.MaxEventsPerRead);
		// if(err < 0) {
		// 	total_err |= err;
		// 	err_msg += "CAEN_DGTZ_SetMaxNumEventsBLT Failed. ";
		// }


		error_wrap("CAEN_DGTZ_SetRecordLength Failed. ",
			CAEN_DGTZ_SetRecordLength,
			handle, res->GlobalConfig.RecordLength);

		// This will calculate what is the max current max buffers
		res->CurrentMaxBuffers = calculate_max_buffers(res);

		error_wrap("CAEN_DGTZ_SetPostTriggerSize Failed. ",
			CAEN_DGTZ_SetPostTriggerSize,
			handle, res->GlobalConfig.PostTriggerPorcentage);

		// Trigger Mode comes in four flavors:
		// CAEN_DGTZ_TRGMODE_DISABLED
		// 		is disabled
		// CAEN_DGTZ_TRGMODE_EXTOUT_ONLY
		// 		is used to generate the trigger output
		// CAEN_DGTZ_TRGMODE_ACQ_ONLY
		//		is used to generate the acquisition trigger
		// CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT
		//		is used to generate the acquisition trigger and trigger output
		error_wrap("CAEN_DGTZ_SetSWTriggerMode Failed. ",
			CAEN_DGTZ_SetSWTriggerMode,
			handle, res->GlobalConfig.SWTriggerMode);

		error_wrap("GetExtTriggerInputMode Failed. ",
			CAEN_DGTZ_SetExtTriggerInputMode,
			handle, res->GlobalConfig.EXTTriggerMode);

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
		error_wrap("CAEN_DGTZ_SetAcquisitionMode Failed. ",
			CAEN_DGTZ_SetAcquisitionMode,
			handle, res->GlobalConfig.AcqMode);

    	// Board config register
    	// 0 = Trigger overlapping not allowed
    	// 1 = trigger overlapping allowed
		error_wrap("write_register to reg 0x8000 Failed. ",
			CAEN_DGTZ_WriteRegister,
			handle, 0x8000,
			((res->GlobalConfig.TriggerOverlappingEn & 0x0001) << 1));

		error_wrap("write_register to reg 0x8100 Failed. ",
			CAEN_DGTZ_WriteRegister,
			handle, 0x8100,
			((res->GlobalConfig.MemoryFullModeSelection & 0x0001) << 5));

		// Channel stuff
		// First, we make the channel mask (if applicable)
		uint32_t channel_mask = 0;
		for(auto ch_config : ch_configs) {
			channel_mask |= 1 << ch_config.Channel;
		}

		// Then enable those channels
		error_wrap("CAEN_DGTZ_SetChannelEnableMask Failed. ",
			CAEN_DGTZ_SetChannelEnableMask,
			handle, channel_mask);

		// then enable their self trigger
		error_wrap("CAEN_DGTZ_SetChannelSelfTrigger Failed. ",
			CAEN_DGTZ_SetChannelSelfTrigger,
			handle, res->GlobalConfig.CHTriggerMode, channel_mask);

		// Let's clear the channel map to allow for new channels properties
		res->ChannelConfigs.clear();
		for(auto ch_config : ch_configs) {

			// Before we set any parameters for each channel, we add the
			// new channel to the map. try_emplace(...) will make sure we do
			// not add duplicates
			auto ins = res->ChannelConfigs.try_emplace(
				ch_config.Channel, ch_config);

			// if false, it was already inserted and we move to the next
			// channel
			if(!ins.second) {
				continue;
			}

	    	// Trigger stuff
	    	// Self Channel trigger
			error_wrap("CAEN_DGTZ_SetChannelTriggerThreshold Failed. ",
				CAEN_DGTZ_SetChannelTriggerThreshold,
				handle, ch_config.Channel, ch_config.TriggerThreshold);

			// DC Offset
			error_wrap("CAEN_DGTZ_SetChannelDCOffset Failed. ",
				CAEN_DGTZ_SetChannelDCOffset,
				handle, ch_config.Channel, ch_config.DCOffset);

        	// Writes to the registers that holds the DC range
        	// For 5730 it is the register 0x1n28
			error_wrap("write_register to reg 0x1n28 Failed (DC Range). " ,
				CAEN_DGTZ_WriteRegister,
				handle, 0x1028 | (ch_config.Channel & 0x0F) << 8,
				ch_config.DCRange & 0x0001);

			// Channel trigger polarity
			error_wrap("CAEN_DGTZ_SetTriggerPolarity Failed. ",
				CAEN_DGTZ_SetTriggerPolarity,
				handle, ch_config.Channel, ch_config.TriggerPolarity);

		}

		if(latest_err < 0) {
			res->LatestError = CAENError {
				.ErrorMessage = "There was en error during setup!" + err_msg,
				.ErrorCode = static_cast<CAEN_DGTZ_ErrorCode>(latest_err),
				.isError = true
			};
		}

	}

	void enable_acquisition(CAEN& res) noexcept {
		if(!res) {
			return;
		}

		if(res->LatestError.isError) {
			return;
		}

		int& handle = res->Handle;

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

	void disable_acquisition(CAEN& res) noexcept {
		if(!res) {
			return;
		}

		auto err = CAEN_DGTZ_SWStopAcquisition(res->Handle);

		if(err < 0) {
			res->LatestError = CAENError {
				.ErrorMessage = "There was en error while trying to disable"
				"acquisition.",
				.ErrorCode = err,
				.isError = true
			};
		}
	}

	void write_register(CAEN& res, uint32_t&& addr,
		const uint32_t& value) noexcept {

		if(!res) {
			return;
		}

		if(res->LatestError.isError) {
			return;
		}

		auto err = CAEN_DGTZ_WriteRegister(res->Handle, addr, value);
		if(err < 0) {
			res->LatestError = CAENError {
				.ErrorMessage = "There was en error while trying to write"
				"to register.",
				.ErrorCode = err,
				.isError = true
			};
		}
	}

	void read_register(CAEN& res, uint32_t&& addr,
		uint32_t& value) noexcept {

		if(!res) {
			return;
		}

		auto err = CAEN_DGTZ_ReadRegister(res->Handle, addr, &value);
		if(err < 0) {
			res->LatestError = CAENError {
				.ErrorMessage = "There was en error while trying to read"
				"to register " + std::to_string(addr),
				.ErrorCode = err,
				.isError = true
			};
		}
	}

	void software_trigger(CAEN& res) noexcept {

		if(!res) {
			return;
		}

		if(res->LatestError.isError) {
			return;
		}

		auto err = CAEN_DGTZ_SendSWtrigger(res->Handle);

		if(err < 0) {
			res->LatestError = CAENError {
				.ErrorMessage = "Failed to send a software trigger",
				.ErrorCode = err,
				.isError = true
			};
		}
	}

	uint32_t get_events_in_buffer(CAEN& res) noexcept {
		uint32_t events = 0;
		// For 5730 it is the register 0x812C
		read_register(res, 0x812C, events);

		return events;
	}

	void retrieve_data(CAEN& res) noexcept {
		int& handle = res->Handle;

		if(!res) {
			return;
		}

		if(res->LatestError.ErrorCode < 0) {
			return;
		}

		int err = CAEN_DGTZ_ReadData(handle,
			CAEN_DGTZ_ReadMode_t::CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,
			res->Data.Buffer,
			&res->Data.DataSize);

		if(err >= 0) {
			err = CAEN_DGTZ_GetNumEvents(handle,
				res->Data.Buffer,
				res->Data.DataSize,
				&res->Data.NumEvents);
		}


	   	if(err < 0) {
			res->LatestError = CAENError {
				.ErrorMessage = "Failed to retrieve data!",
				.ErrorCode = static_cast<CAEN_DGTZ_ErrorCode>(err),
				.isError = true
			};
	   	}
	}

	bool retrieve_data_until_n_events(CAEN& res, uint32_t&& n) noexcept {
		int& handle = res->Handle;

		if(!res) {
			return false;
		}

		if(n > res->CurrentMaxBuffers) {
			if(get_events_in_buffer(res) < res->CurrentMaxBuffers) {
				return false;
			}
		} else if(get_events_in_buffer(res) < n) {
			return false;
		}


		if(res->LatestError.ErrorCode < 0) {
			return false;
		}

		// There is a weird quirk related to this function that BLOCKS
		// the software essentially killing it.
		// If TriggerOverlapping is allowed sometimes this function
		// returns more data than Buffer it can hold, why? Idk
		// but so far with the software as is, it won't work with that
		// so dont do it!
		res->LatestError.ErrorCode = CAEN_DGTZ_ReadData(handle,
			CAEN_DGTZ_ReadMode_t::CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,
			res->Data.Buffer,
			&res->Data.DataSize);

		if(res->LatestError.ErrorCode < 0){
			res->LatestError.isError = true;
			res->LatestError.ErrorMessage = "There was an error when trying "
			"to read buffer";
			return false;
		}

		res->LatestError.ErrorCode = CAEN_DGTZ_GetNumEvents(handle,
				res->Data.Buffer,
				res->Data.DataSize,
				&res->Data.NumEvents);

		if(res->LatestError.ErrorCode < 0){
			res->LatestError.isError = true;
			res->LatestError.ErrorMessage = "There was an error when trying"
			"to recover number of events from buffer";
			return false;
		}

		return true;
	}

	void extract_event(CAEN& res, const uint32_t& i, CAENEvent& evt) noexcept {
		int& handle = res->Handle;

		if(!res ) {
			return;
		}

		if(res->LatestError.ErrorCode < 0 ){
			return;
		}

		if(!evt) {
			evt = std::make_shared<caenEvent>(handle);
		}

	    CAEN_DGTZ_GetEventInfo(handle,
	    	res->Data.Buffer,
	    	res->Data.DataSize,
	    	i,
	    	&evt->Info,
	    	&evt->DataPtr);

	    CAEN_DGTZ_DecodeEvent(handle,
    		evt->DataPtr,
    		reinterpret_cast<void**>(&evt->Data));
	}

	void clear_data(CAEN& res) noexcept {

		if(!res) {
			return;
		}

		int& handle = res->Handle;
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

	// uint32_t channel_self_trigger_meter(CAEN& res, uint8_t channel) noexcept {
	// 	uint32_t freq = 0;
	// 	// For 5730 it is 0x1nEC where n is the channel
	// 	read_register(res, 0x10EC | (channel & 0x0F) << 8,
 //        		freq);

	// 	return freq;
	// }

	uint32_t t_to_record_length(CAEN& res, double nsTime) noexcept {
		return static_cast<uint32_t>(nsTime*1e-9*res->GetCommTransferRate());
	}

	// Turns a voltage (V) into trigger counts
	uint32_t v_to_threshold_counts(CAEN& res, double volt) noexcept {
		return 0;
	}

	// Turns a voltage (V) into count offset
	uint32_t v_offset_to_count_offset(CAEN&, double volt) noexcept {
		return 0;
	}

	// Returns the max number of buffers for the given
	// record length in globalconfig, and memory per channels
	uint32_t calculate_max_buffers(CAEN& res) noexcept {
		// This calculation is trickier than it seems actually
		// first, naively it can be calculated as such
		// 		Buffers = Memory per channel / Record length
		// but internally record length can only be a power of 2
		// or multiples of 10 so we need to check for those cases
		uint32_t rl = res->GlobalConfig.RecordLength;

		// Cannot be higher than the memory per channel
		auto mem_per_ch = res->GetMemoryPerChannel();
		if(rl >= mem_per_ch) {
			rl = mem_per_ch;
		}

		//
		uint32_t fake_num_buffs = mem_per_ch / rl;
		// if fake_num_buffs is a power of 2 then we can use it as is
		// https://stackoverflow.com/questions/108318/how-can-i-test-whether-a-number-is-a-power-of-2
		// otherwise the minimum it can be is 10
		// and only be multiple of 10s
		if((fake_num_buffs & (fake_num_buffs - 1)) != 0) {
			if(rl > 10.0) {
				double tmp_rl_double = rl / 10.0;
				rl = 10*round(tmp_rl_double);
			} else {
				rl = 10;
			}
		}

		// Then we calculate the actual num buffs but...
		auto max_num_buffs = mem_per_ch / rl;

		// It cannot  be higher than the max buffers
		max_num_buffs = max_num_buffs >= res->GetMaxNumberOfBuffers() ?
			res->GetMaxNumberOfBuffers() : max_num_buffs;

		// It can only be a power of 2
		uint32_t real_max_buffs = static_cast<uint32_t>(exp2(floor(log2(max_num_buffs))));

		// Unless this is enabled, then it is always that number minus one.
		// Except when the buffers is 1
		if(res->GlobalConfig.MemoryFullModeSelection) {
			if(real_max_buffs > 1) {
				real_max_buffs--;
			} else {
				real_max_buffs = 2;
			}
		}

		// Phew, we are done!
		return real_max_buffs;
	}

	std::string sbc_init_file(CAEN& res) noexcept {
		// header string = name;type;x,y,z...;
		auto g_config = res->GlobalConfig;
		auto ch_configs = res->ChannelConfigs;
		auto sample_rate = res->GetSampleRate();

		std::string total = "";

		auto NumToBinString = [] (auto num) -> std::string {
			char* tmpstr = reinterpret_cast<char*>(&num);
			return std::string(tmpstr, sizeof(num) / sizeof(char));
		};

		// Lambda to help to save a single number to the binary format
		// file. The numbers are always converted to double
		auto SingleNumToBinary = [&] (std::string name, auto num)
			-> std::string {

			auto data = NumToBinString(static_cast<double>(num));
			auto header = name + ";double;1;";

			uint16_t s = header.length();
			auto header_length = NumToBinString(s);

			int32_t lines = 1;
			auto num_lines = NumToBinString(lines);

			return header_length + header + num_lines + data;

		};

		// Same as above but for arrays
		auto ArrayToBinary = [&] (std::string name, auto nums)
			-> std::string {

			// auto data = NumToBinString(num);
			auto header = name + ";double;"
				+ std::to_string(nums.size()) + ";";

			uint16_t s = header.length();
			auto header_length = NumToBinString(s);

			int32_t lines = 1;
			auto num_lines = NumToBinString(lines);

			std::string data = "";
			for(auto num : nums) {
				data += NumToBinString(static_cast<double>(num));
			}

			return header_length + header + num_lines + data;

		};

		uint32_t l = 0x01020304;
		std::string endianess = NumToBinString(l);

		total = endianess;
		total += SingleNumToBinary("sample_rate", sample_rate);

		std::vector<double> channels;
		std::vector<double> thresholds;
		std::vector<double> dc_offset;
		std::vector<double> dc_range;
		for(auto config : ch_configs) {

			auto ch = config.first;

			channels.emplace_back(ch);
			thresholds.emplace_back(config.second.TriggerThreshold);
			dc_offset.emplace_back(config.second.DCOffset);
			dc_range.emplace_back(res->GetVoltageRange(ch));

		}

		total += ArrayToBinary("Channels", channels);
		total += ArrayToBinary("Thresholds", thresholds);
		total += ArrayToBinary("DCOffset", dc_offset);
		total += ArrayToBinary("DCRange", dc_range);

		// This part is the header for the SiPM pulses
		// The pulses are saved as raw counts, so uint16 is enough
		const std::string c_type = "uint16";
		// Name of this block
		const std::string c_sipm_name = "SIPM_traces";

		// These are all the data line dimensions
		// 1xRecordLengthxNumChannels
		std::string time_stamp = std::to_string(1);
		std::string record_length = std::to_string(g_config.RecordLength);
		std::string num_channels = std::to_string(ch_configs.size());

		std::string data_header = c_sipm_name + ";";	// name
		data_header += c_type + ";";					// type
		data_header += time_stamp + ",";				// dim 1
		data_header += record_length + ",";				// dim 2
		data_header += num_channels + ",";				// dim 3

		// uint16_t is required as the format requires it to be 16 unsigned max
		uint16_t s = data_header.length();
		std::string data_header_size = NumToBinString(s);

		// 0 means that it will be calculated by the number of lines
		// int32 is required
		int32_t j = 0x00000000;
		std::string num_data_lines = NumToBinString(j);

		return total + data_header_size + data_header + num_data_lines;
	}

	// std::string sbc_init_file(CAENEvent& evt) noexcept {

	// 	uint16_t numchannels = 0;
	// 	uint16_t rl = 0;
	// 	// We need  the channels that are enabled but for that it is actually
	// 	// easier to count all the channels that have a size different than 0
	// 	for(int i = 0; i < MAX_UINT16_CHANNEL_SIZE; i++){
	// 		if(evt->Data->ChSize[i] > 0) {
	// 			numchannels++;
	// 			// all evt->Data->ChSize should be the same size...
	// 			rl = evt->Data->ChSize[i];
	// 		}
	// 	}

	// 	return sbc_init_file(rl, numchannels);
	// }


	std::string sbc_save_func(CAENEvent& evt) noexcept {

		char* ptr = nullptr;
		std::string out_data = "";
		// This is how it works:
		// the SBC saving format (AKA binary recon data format) works like
		// the next:
		// 1.- 	There is an endianness indicator. That is why sbc_init_file
		// 		exists.
		// 2.- 	then a header string length, which is how long in # of chars
		// 		the entire header string is
		// 3.-	The header string. This holds the name of the block, the length
		//		of each dimension of the array and its type
		// 4.- 	The number of lines of vectors in each array

		// For CAEN data, each line is an Event which contains a 2-D array
		// where the x-axis is the record length and the y-axis are the
		// number of channels that are activated

		auto& evtdata = evt->Data;
		for(int ch = 0; ch < MAX_UINT16_CHANNEL_SIZE; ch++){
			if(evtdata->ChSize[ch] > 0) {

				out_data += std::string();

				for(uint32_t xp = 0; xp < evtdata->ChSize[ch]; xp++) {

					ptr = reinterpret_cast<char*>(&evtdata->DataChannel[ch][xp]);
					out_data += std::string(ptr, 2);

				}
			}
		}

		return out_data;
	}

} // namespace SBCQueens