#include "caen_helper.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <spdlog/spdlog.h>
#include <bitset>
#include <iostream>
#include <functional>

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

        void setup(CAEN &res, CAENGlobalConfig g_config,
                   std::vector<CAENGroupConfig> gr_configs) noexcept {

          if (!res) {
            return;
          }

          if (res->LatestError.isError) {
            return;
          }

          reset(res);
          int &handle = res->Handle;
          int latest_err = 0;
          std::string err_msg = "";

          // This lambda wraps whatever CAEN function you pass to it,
          // checks the error, and if there was an error, add the error msg to
          // it If there is an error, it also does not execute the function.
          auto error_wrap = [&](std::string msg, auto f, auto... args) {
            if (latest_err >= 0) {
              auto err = f(args...);
              if (err < 0) {
                latest_err = err;
                err_msg += msg;
              }
            }
          };

          // Global config
          res->GlobalConfig = g_config;
          error_wrap("CAEN_DGTZ_SetMaxNumEventsBLT Failed. ",
                     CAEN_DGTZ_SetMaxNumEventsBLT, handle,
                     res->GlobalConfig.MaxEventsPerRead);
          // Before:
          // err = CAEN_DGTZ_SetMaxNumEventsBLT(handle,
          //	res->GlobalConfig.MaxEventsPerRead);
          // if(err < 0) {
          // 		total_err |= err;
          // 		err_msg += "CAEN_DGTZ_SetMaxNumEventsBLT Failed. ";
          // }

          error_wrap("CAEN_DGTZ_SetRecordLength Failed. ",
                     CAEN_DGTZ_SetRecordLength, handle,
                     res->GlobalConfig.RecordLength);

          // This will calculate what is the max current max buffers
          res->CurrentMaxBuffers = calculate_max_buffers(res);

          // This will get the ACTUAL record length as calculated by
          // CAEN_DGTZ_SetRecordLength
          uint32_t nloc = 0;
          error_wrap("Failed to read NLOC. ", CAEN_DGTZ_ReadRegister, handle,
                     0x8020, &nloc);
          res->GlobalConfig.RecordLength = res->GetNLOCTORecordLength() * nloc;

          error_wrap("CAEN_DGTZ_SetPostTriggerSize Failed. ",
                     CAEN_DGTZ_SetPostTriggerSize, handle,
                     res->GlobalConfig.PostTriggerPorcentage);

          // Trigger Mode comes in four flavors:
          // CAEN_DGTZ_TRGMODE_DISABLED
          // 		is disabled
          // CAEN_DGTZ_TRGMODE_EXTOUT_ONLY
          // 		is used to generate the trigger output
          // CAEN_DGTZ_TRGMODE_ACQ_ONLY
          //		is used to generate the acquisition trigger
          // CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT
          //		is used to generate the acquisition trigger and trigger
          //output
          error_wrap("CAEN_DGTZ_SetSWTriggerMode Failed. ",
                     CAEN_DGTZ_SetSWTriggerMode, handle,
                     res->GlobalConfig.SWTriggerMode);

          error_wrap("CAEN_DGTZ_SetExtTriggerInputMode Failed. ",
                     CAEN_DGTZ_SetExtTriggerInputMode, handle,
                     res->GlobalConfig.EXTTriggerMode);

          // Acquisition mode comes in four flavors:
          // CAEN_DGTZ_SW_CONTROLLED
          // 		Start/stop of the run takes place on software
          // 		command (by calling CAEN_DGTZ_SWStartAcquisition )
          // CAEN_DGTZ_FIRST_TRG_CONTROLLED
          // 		Run starts on the first trigger pulse (rising edge on
          // TRG-IN) 		actual triggers start from the second pulse from there
          // CAEN_DGTZ_S_IN_CONTROLLED
          //		S-IN/GPI controller (depends on the board). Acquisition
          // 		starts on edge of the GPI/S-IN
          // CAEN_DGTZ_LVDS_CONTROLLED
          //		VME ONLY, like S_IN_CONTROLLER but uses LVDS.
          error_wrap("CAEN_DGTZ_SetAcquisitionMode Failed. ",
                     CAEN_DGTZ_SetAcquisitionMode, handle,
                     res->GlobalConfig.AcqMode);

          // Trigger polarity
          // These digitizers do not support channel-by-channel trigger pol
          // so we treat it like a global config, and use 0 as a placeholder.
          error_wrap("CAEN_DGTZ_SetTriggerPolarity Failed. ",
                     CAEN_DGTZ_SetTriggerPolarity, handle, 0,
                     res->GlobalConfig.TriggerPolarity);

          error_wrap("CAEN_DGTZ_SetIOLevel Failed. ",
                     CAEN_DGTZ_SetIOLevel, handle,
                     res->GlobalConfig.IOLevel);

          // Board config register
          // 0 = Trigger overlapping not allowed
          // 1 = trigger overlapping allowed
          write_bits(res, 0x8000, res->GlobalConfig.TriggerOverlappingEn, 1);

          write_bits(res, 0x8100, res->GlobalConfig.MemoryFullModeSelection, 5);

          // Channel stuff
          if (res->Model == CAENDigitizerModel::DT5730B) {
            // For DT5730B, there are no groups only channels so we take
            // each configuration as a channel

            // First, we make the channel mask (if applicable)
            uint32_t channel_mask = 0;
            for (auto ch_config : gr_configs) {
              channel_mask |= 1 << ch_config.Number;
            }

            // Then enable those channels
            error_wrap("CAEN_DGTZ_SetChannelEnableMask Failed. ",
                       CAEN_DGTZ_SetChannelEnableMask, handle, channel_mask);

            // Then enable their self trigger
            error_wrap("CAEN_DGTZ_SetChannelSelfTrigger Failed. ",
                       CAEN_DGTZ_SetChannelSelfTrigger, handle,
                       res->GlobalConfig.CHTriggerMode, channel_mask);

            // Let's clear the channel map to allow for new channels properties
            res->GroupConfigs.clear();
            for (auto ch_config : gr_configs) {
              // Before we set any parameters for each channel, we add the
              // new channel to the map. try_emplace(...) will make sure we do
              // not add duplicates
              auto ins =
                  res->GroupConfigs.try_emplace(ch_config.Number, ch_config);

              // if false, it was already inserted and we move to the next
              // channel
              if (!ins.second) {
                continue;
              }

              // Trigger stuff
              // Self Channel trigger
              error_wrap("CAEN_DGTZ_SetChannelTriggerThreshold Failed. ",
                         CAEN_DGTZ_SetChannelTriggerThreshold, handle,
                         ch_config.Number, ch_config.TriggerThreshold);

              error_wrap("CAEN_DGTZ_SetChannelDCOffset Failed. ",
                         CAEN_DGTZ_SetChannelDCOffset, handle, ch_config.Number,
                         ch_config.DCOffset);

              // Writes to the registers that holds the DC range
              // For 5730 it is the register 0x1n28
              error_wrap("write_register to reg 0x1n28 Failed (DC Range). ",
                         CAEN_DGTZ_WriteRegister, handle,
                         0x1028 | (ch_config.Number & 0x0F) << 8,
                         ch_config.DCRange & 0x0001);
            }
          } else if (res->Model == CAENDigitizerModel::DT5740D) {
            // For DT5740D

            uint32_t group_mask = 0;
            for (auto gr_config : gr_configs) {
              group_mask |= 1 << gr_config.Number;
            }

            error_wrap("CAEN_DGTZ_SetGroupEnableMask Failed. ",
                       CAEN_DGTZ_SetGroupEnableMask, handle, group_mask);

            error_wrap("CAEN_DGTZ_SetGroupSelfTrigger Failed. ",
                       CAEN_DGTZ_SetGroupSelfTrigger, handle,
                       res->GlobalConfig.CHTriggerMode, group_mask);

            res->GroupConfigs.clear();
            for (auto gr_config : gr_configs) {

              // Before we set any parameters for each channel, we add the
              // new channel to the map. try_emplace(...) will make sure we do
              // not add duplicates
              auto ins =
                  res->GroupConfigs.try_emplace(gr_config.Number, gr_config);

              // if false, it was already inserted and we move to the next
              // channel
              if (!ins.second) {
                continue;
              }

              // Trigger stuff
              error_wrap("CAEN_DGTZ_SetGroupTriggerThreshold Failed. ",
                         CAEN_DGTZ_SetGroupTriggerThreshold, handle,
                         gr_config.Number, gr_config.TriggerThreshold);

              error_wrap("CAEN_DGTZ_SetGroupDCOffset Failed. ",
                         CAEN_DGTZ_SetGroupDCOffset, handle, gr_config.Number,
                         gr_config.DCOffset);

              // Set the mask for channels enabled for self-triggering
              error_wrap("CAEN_DGTZ_SetChannelGroupMask Failed. ",
              	CAEN_DGTZ_SetChannelGroupMask,
              	handle, gr_config.Number, gr_config.TriggerMask);

              // Set acquisition mask
              write_bits(res, 0x10A8 | (gr_config.Number << 8), 
              	gr_config.AcquisitionMask, 0, 8);

              // DCCorrections should be of length
              // NumberofChannels / NumberofGroups.
              // set individual channel 8-bitDC offset
              // on 12 bit LSB scale, same as threshold
              uint32_t word = 0;
              for (int ch = 0; ch < 4; ch++) {
                word += gr_config.DCCorrections[ch] << (ch * 8);
              }
              write_register(res, 0x10C0 | (gr_config.Number << 8), word);
              word = 0;
              for (int ch = 4; ch < 8; ch++) {
                word += gr_config.DCCorrections[ch] << ((ch - 4) * 8);
              }
              write_register(res, 0x10C4 | (gr_config.Number << 8), word);
            }


            bool trg_out = false;

            // For 740, to use TRG-IN as Gate / anti-veto
            if (g_config.EXTasGate) {
            	write_bits(res, 0x810C, 1, 27); // TRG-IN AND internal trigger, to serve as gate
            	write_bits(res, 0x811C, 1, 10); // TRG-IN as gate
            }

            if (trg_out) {
            	write_bits(res, 0x811C, 0, 15); // TRG-OUT based on internal signal
            	write_bits(res, 0x811C, 0b00, 16, 2); // TRG-OUT based on internal signal
            }
            write_bits(res, 0x811C, 0b01, 21, 2);

            // read_register(res, 0x8110, word);
            // word |= 1; // enable group 0 to participate in GPO
            // write_register(res, 0x8110, word);

          } else {
            // custom error message if not above models
            latest_err = -1;
            err_msg += "Model not supported.";
          }

          if (latest_err < 0) {
            res->LatestError = CAENError{
                .ErrorMessage = "There was en error during setup!" + err_msg,
                .ErrorCode = static_cast<CAEN_DGTZ_ErrorCode>(latest_err),
                .isError = true};
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

	void write_bits(CAEN& res, uint32_t&& addr,
		const uint32_t& value, uint8_t pos, uint8_t len) noexcept {

		if(!res) {
			return;
		}

		// First read the register
		uint32_t read_word = 0;
		auto err = CAEN_DGTZ_ReadRegister(res->Handle, addr, &read_word);

		if(err < 0) {
			res->LatestError = CAENError {
				.ErrorMessage = "There was en error while trying to read"
				"to register " + std::to_string(addr),
				.ErrorCode = err,
				.isError = true
			};
		}

		uint32_t bit_mask = ~(((1<<len) - 1) << pos);
		read_word = read_word & bit_mask; //mask the register value

		// Get the lowest bits of value and shifted to the correct position
		uint32_t value_bits = (value & ((1<<len) - 1)) << pos;
		// Combine masked value read from register with new bits
		err = CAEN_DGTZ_WriteRegister(res->Handle, addr, read_word | value_bits);

		if(err < 0) {
			res->LatestError = CAENError {
				.ErrorMessage = "There was en error while trying to write"
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

	// Calculates the number of buffers that are going to be used
	// given current settings.
	uint32_t calculate_max_buffers(CAEN& res) noexcept {

		// Instead of using the stored RecordLength
		// We will let CAEN functions do all the hard work for us
		// uint32_t rl = res->GlobalConfig.RecordLength;
		uint32_t rl = 0;

		// Cannot be higher than the memory per channel
		auto mem_per_ch = res->GetMemoryPerChannel();
		uint32_t nloc = 0;

		// This contains nloc which if multiplied by the correct
		// constant, will give us the record length exactly
		read_register(res, 0x8020, nloc);

		try {
			rl =  res->GetNLOCTORecordLength()*nloc;
		} catch (...) {
			return 0;
		}

		// Then we calculate the actual num buffs but...
		auto max_num_buffs = mem_per_ch / rl;

		// It cannot be higher than the max buffers
		max_num_buffs = max_num_buffs >= res->GetMaxNumberOfBuffers() ?
			res->GetMaxNumberOfBuffers() : max_num_buffs;

		// It can only be a power of 2
		uint32_t real_max_buffs
			= static_cast<uint32_t>(exp2(floor(log2(max_num_buffs))));

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
		auto group_configs = res->GroupConfigs;

		// The enum and the map is to simplify the code for the lambdas.
		// and the header could potentially change any moment with any type
		enum class Types { CHAR, INT8, INT16, INT32, INT64, UINT8, UINT16,
		UINT32, UINT64, SINGLE, FLOAT, DOUBLE, FLOAT128 };

		const std::unordered_map<Types, std::string> type_to_string = {
			{Types::CHAR, "char"},
			{Types::INT8, "int8"}, {Types::INT16, "int16"},
			{Types::INT32, "int32"}, {Types::UINT8, "uint8"},
			{Types::UINT16, "uint16"}, {Types::UINT32, "uint32"},
			{Types::SINGLE, "single"}, {Types::FLOAT, "single"},
			{Types::DOUBLE, "double"}, {Types::FLOAT128, "float128"}
		};

		auto NumToBinString = [] (auto num) -> std::string {
			char* tmpstr = reinterpret_cast<char*>(&num);
			return std::string(tmpstr, sizeof(num) / sizeof(char));
		};

		// Lambda to help to save a single number to the binary format
		// file. The numbers are always converted to double
		auto SingleDimToBinaryHeader = [&] (std::string name, Types type,
			uint32_t length) -> std::string {

			auto type_s = type_to_string.at(type);
			auto header = name + ";" + type_s + ";" + std::to_string(length) + ";";

			return header;
		};
		
		uint32_t l = 0x01020304;
		auto endianness_s = NumToBinString(l);

		std::string header = SingleDimToBinaryHeader("sample_rate", Types::DOUBLE, 1);

		// TODO(Zhiheng): ch_configs.size() should not be used for your
		// case. Calculate the number of channels using the mask and number
		// of groups
		std::function<uint32_t (uint32_t)> n_channels_acq = [&](uint8_t acq_mask) {
			return acq_mask==0 ? 0: (acq_mask & 1) + n_channels_acq(acq_mask>>1);
		};
		uint32_t num_ch = 0;
		for (auto gr_pair : res->GroupConfigs) {
			num_ch += n_channels_acq(gr_pair.second.AcquisitionMask);
		}

		header += SingleDimToBinaryHeader(
			"en_chs", Types::UINT8, num_ch
		);

		header += SingleDimToBinaryHeader(
			"trg_mask", Types::UINT32, 1
		);

		header += SingleDimToBinaryHeader(
			"thresholds", Types::UINT32, num_ch
		);

		header += SingleDimToBinaryHeader(
			"dc_offsets", Types::UINT32, num_ch
		);

		header += SingleDimToBinaryHeader(
			"dc_corrections", Types::UINT8, num_ch
		);

		header += SingleDimToBinaryHeader(
			"dc_range", Types::SINGLE, num_ch
		);

		header += SingleDimToBinaryHeader(
			"time_stamp", Types::UINT32, 1
		);

		header += SingleDimToBinaryHeader(
			"trg_source", Types::UINT32, 1
		);

		// This part is the header for the SiPM pulses
		// The pulses are saved as raw counts, so uint16 is enough
		const std::string c_type = "uint16";
		// Name of this block
		const std::string c_sipm_name = "sipm_traces";

		// These are all the data line dimensions
		// RecordLengthxNumChannels
		std::string record_length_s = std::to_string(g_config.RecordLength);
		std::string num_channels_s = std::to_string(num_ch);

		std::string data_header = c_sipm_name + ";";	// name
		data_header += c_type + ";";					// type
		data_header += record_length_s + ",";				// dim 1
		data_header += num_channels_s + ";";					// dim 2

		// uint16_t is required as the format requires it to be 16 unsigned max
		uint16_t s = (header + data_header).length();
		std::string data_header_size = NumToBinString(s);

		// 0 means that it will be calculated by the number of lines
		// int32 is required
		int32_t j = 0x00000000;
		std::string num_data_lines = NumToBinString(j);

		// binary format goes like: endianess, header size, header string
		// then number of lines (in this case 0)
		return endianness_s + data_header_size + header + data_header + num_data_lines;
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


	std::string sbc_save_func(CAENEvent& evt, CAEN& res) noexcept {

		// TODO(Hector): so this code could be improved and half of the reason
		// it is related to the format itself. A bunch of the details
		// it is saving with each line should not be saved every time. They are
		// run constants, but that is how it is for now. Maybe in the future
		// it will change.

		// TODO(Zhiheng): add the code you need to save the channels on
		// a group by using the mask.

		// order of header:
		// Name				type		length (B) 		is Constant?
		// sample_rate		double		8 	 			Y
		// en_chs			uint8		1*ch_size 		Y
		// trg_mask			uint32		4 		 		Y
		// thresholds 		uint32 		4*ch_size 		Y
		// dc_offsets 		uint32 		4*ch_size 		Y
		// dc_corrections	uint8 		1*ch_size 		Y
		// dc_range 		single 		4*ch_size 		Y
		// time_stamp 		uint32 		4 				N
		// trg_source 		uint32 		4 				N
		// data 			uint16 		2*rl*ch_size 	N
		//
		// Total length 				20 + ch_size(14 + 2*recordlength)

		std::function<uint32_t (uint32_t)> n_channels_acq = [&](uint8_t acq_mask) {
			return acq_mask==0 ? 0: (acq_mask & 1) + n_channels_acq(acq_mask>>1);
		};
		uint32_t num_ch = 0;
		for (auto gr_pair : res->GroupConfigs) {
			num_ch += n_channels_acq(gr_pair.second.AcquisitionMask);
		}
		const auto rl = res->GlobalConfig.RecordLength;

		const size_t nline = 20 + (2*rl + 14)*num_ch;

		const uint8_t ch_per_group = res->GetNumberOfChannelsPerGroup();



		// No strings for this one as this is more efficient
		char out_str[nline];

		// double
		uint64_t offset = 0;
		auto append_cstr = [](auto num, uint64_t& offset, char* str) {
			char* ptr = reinterpret_cast<char*>(&num);
			for(size_t i = 0; i < sizeof(num) / sizeof(char); i++) {
				str[offset + i] = ptr[i];
			}
			offset += sizeof(num) / sizeof(char);
		};

		// sample_rate
		append_cstr(res->GetSampleRate(), offset, &out_str[0]);

		// en_chs
		for(auto gr_pair : res->GroupConfigs) {
			for(int ch = 0; ch < ch_per_group; ch++) {
				if (gr_pair.second.AcquisitionMask & (1<<ch)){
					uint8_t channel = gr_pair.second.Number * ch_per_group + ch;
					append_cstr(channel, offset, &out_str[0]);
				}
			}
		}

		// trg_mask
		uint32_t trg_mask = 0;
		for(auto gr_pair : res->GroupConfigs) {
			trg_mask |= (gr_pair.second.TriggerMask << 
				(gr_pair.second.Number * ch_per_group));
		}
		append_cstr(trg_mask, offset, &out_str[0]);

		// thresholds
		for(auto gr_pair : res->GroupConfigs) {
			for(int ch = 0; ch < ch_per_group; ch++) {
				if (gr_pair.second.AcquisitionMask & (1<<ch)){
					append_cstr(gr_pair.second.TriggerThreshold, offset, &out_str[0]);
				}
			}
		}

		// dc_offsets
		for(auto gr_pair : res->GroupConfigs) {
			for(int ch = 0; ch < ch_per_group; ch++) {
				if (gr_pair.second.AcquisitionMask & (1<<ch)){
					append_cstr(gr_pair.second.DCOffset, offset, &out_str[0]);
				}
			}
		}

		// dc_corrections
		for(auto gr_pair : res->GroupConfigs) {
			for(int ch = 0; ch < ch_per_group; ch++) {
				if (gr_pair.second.AcquisitionMask & (1<<ch)){
					append_cstr(gr_pair.second.DCCorrections[ch], offset, &out_str[0]);
				}
			}
		}

		// dc_range
		for(auto gr_pair : res->GroupConfigs) {
			for(int ch = 0; ch < ch_per_group; ch++) {
				if (gr_pair.second.AcquisitionMask & (1<<ch)){
					float val = res->GetVoltageRange(gr_pair.second.Number);
					append_cstr(val, offset, &out_str[0]);
				}
			}
		}

		// time_stamp
		append_cstr(evt->Info.TriggerTimeTag, offset, &out_str[0]);

		// tgr_source
		append_cstr(evt->Info.Pattern, offset, &out_str[0]);


		// For CAEN data, each line is an Event which contains a 2-D array
		// where the x-axis is the record length and the y-axis are the
		// number of channels that are activated
		auto& evtdata = evt->Data;
		for(auto gr_pair : res->GroupConfigs){
			auto& gr = gr_pair.second.Number;
			for(int ch = 0; ch < ch_per_group; ch++) {
				if (gr_pair.second.AcquisitionMask & (1<<ch)){
					for(uint32_t xp = 0; xp < evtdata->ChSize[gr*ch_per_group+ch]; xp++) {
						append_cstr(evtdata->DataChannel[ch][xp], offset, &out_str[0]);
					}
				}
			}
		}

		// However, we do convert to string at the end, I wonder if this
		// is a big performance impact?
		return std::string(out_str, nline);
	}

} // namespace SBCQueens
