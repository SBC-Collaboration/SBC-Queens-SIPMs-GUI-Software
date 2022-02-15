#pragma once

// std includes
#include "CAENComm.h"
#include <cstdint>
#include <cwchar>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <cmath>

// 3rd party includes
#include <CAENDigitizer.h>


// my includes

namespace SBCQueens {

	enum class CAENDigitizerModel {
		DT5730B = 0,
		DT5740D = 1
	};

	// All the constants that never change between digitizers
	struct CAENDigitizerModelConstants {
		// ADC resolution in bits
		uint32_t ADCResolution;
		// In S/s
		double AcquisitionRate;
		// In S/ch
		uint32_t MemoryPerChannel;
		// In S/s
		uint32_t USBTransferRate = 15e6;

		uint32_t NumChannels = 1;

		uint32_t MaxNumBuffers = 1024;

		float NLOCToRecordLength = 1;

		std::vector<double> VoltageRanges;
	};

	// This is here so we can transform string to enums
	const std::unordered_map<std::string, CAENDigitizerModel>
		CAENDigitizerModels_map {
			{"DT5730B", CAENDigitizerModel::DT5730B},
			{"DT5740D", CAENDigitizerModel::DT5740D}
	};

	// These links all the enums with their constants or properties that
	// are fixed per digitizer
	const std::unordered_map<CAENDigitizerModel, CAENDigitizerModelConstants>
		CAENDigitizerModelsConstants_map {
			{CAENDigitizerModel::DT5730B, CAENDigitizerModelConstants{
				.ADCResolution = 14,
				.AcquisitionRate = 500e6,
				.MemoryPerChannel = static_cast<uint32_t>(5.12e6),
				.NumChannels = 8,
				.NLOCToRecordLength = 10,
				.VoltageRanges = {0.5, 2.0}
			}},
			{CAENDigitizerModel::DT5740D, CAENDigitizerModelConstants{
				.ADCResolution = 12,
				.AcquisitionRate = 62.5e6,
				.MemoryPerChannel = static_cast<uint32_t>(192e3),
				.NumChannels = 32,
				.NLOCToRecordLength = 1.5,
				.VoltageRanges = {2.0, 10.0}
			}}
	};

	// Struct that holds all the items an error might hold.
	struct CAENError {
		std::string ErrorMessage = "";
		CAEN_DGTZ_ErrorCode ErrorCode = CAEN_DGTZ_ErrorCode::CAEN_DGTZ_Success;
		bool isError = false;
	};

	struct CAENGlobalConfig {
		// X730 max buffers is 1024
		// X740 max buffers is 1024
		uint32_t MaxEventsPerRead = 512;

		// Record length in samples
		uint32_t RecordLength = 400;

		// In %
		uint32_t PostTriggerPorcentage = 50;

		// see caen_helper.cpp setup(...) to see a description of this
		CAEN_DGTZ_TriggerMode_t EXTTriggerMode
			= CAEN_DGTZ_TriggerMode_t::CAEN_DGTZ_TRGMODE_ACQ_ONLY;

		CAEN_DGTZ_TriggerMode_t SWTriggerMode
			= CAEN_DGTZ_TriggerMode_t::CAEN_DGTZ_TRGMODE_ACQ_ONLY;

		CAEN_DGTZ_TriggerMode_t CHTriggerMode
			= CAEN_DGTZ_TriggerMode_t::CAEN_DGTZ_TRGMODE_ACQ_ONLY;

		CAEN_DGTZ_AcqMode_t AcqMode
			= CAEN_DGTZ_AcqMode_t::CAEN_DGTZ_SW_CONTROLLED;

		// This feature is for X730
		// True = disabled, False = enabled
		bool TriggerOverlappingEn = false;

		// 0 = normal, the board is full whenever all buffers are full
		// 1 = One buffer free. The board is full whenever Nb-1 buffers
		// are full, where Nb is the overall number of numbers of buffers
		// in which the channel memory is divided.
		bool MemoryFullModeSelection = true;
	};

	struct CAENChannelConfig {
		// channel #
		uint8_t Channel = 0;

		// 0x8000 no offset if 16 bit DAC
		// 0x1000 no offset if 14 bit DAC
		// 0x400 no offset if 14 bit DAC
		uint32_t DCOffset = 0x8000;

		// For DT5730
		// 0 = 2Vpp, 1 = 0.5Vpp
		uint8_t DCRange = 0;

		// In ADC counts
		uint32_t TriggerThreshold = 0;

		// 0L = Rising edge, 1L = Falling edge
		CAEN_DGTZ_TriggerPolarity_t TriggerPolarity =
			CAEN_DGTZ_TriggerPolarity_t::CAEN_DGTZ_TriggerOnRisingEdge;
	};

	// Holds the CAEN raw data, the size of the buffer, the size
	// of the data and number of events
	struct CAENData {
		char* Buffer = nullptr;
		uint32_t TotalSizeBuffer;
		uint32_t DataSize;
		uint32_t NumEvents;
	};

	// Events structure: holds the raw data of the event, the info (timestamp),
	// and the pointer to the point in the original buffer. No copying
	// or moving allowed as the data structure is inside a C (insecure) pointer
	// that is allocated by CAEN functions
	// caenEvent can be called normally and its constructors and destructors
	// will make sure the memory is freed
	struct caenEvent {
		// Treat this like a shared pointer
		char* DataPtr;
		// This is a "shared_ptr"
		CAEN_DGTZ_UINT16_EVENT_t* Data = nullptr;
		CAEN_DGTZ_EventInfo_t Info;

		explicit caenEvent(const int& handle)
			: _handle(handle) {
			CAEN_DGTZ_AllocateEvent(_handle,
				reinterpret_cast<void**>(&Data));
		}

		// If *handle is released before this event is freed,
		// it will cause a memory leak, maybe?
		~caenEvent() {
			CAEN_DGTZ_FreeEvent(_handle,
				reinterpret_cast<void**>(&Data) );
		}

		// This copies event other into this
		caenEvent(const caenEvent& other) {
			this->_handle = other._handle;
			this->DataPtr = other.DataPtr;
		 	CAEN_DGTZ_AllocateEvent(other._handle,
				reinterpret_cast<void**>(&this->Data));
		 	*this->Data = *other.Data;
		 	this->Info = other.Info;

		}

		caenEvent operator=(const caenEvent& other) {
			return caenEvent(other);
		}


private:
		int _handle;

	};
	using CAENEvent = std::shared_ptr<caenEvent>;

	struct caen {

		caen(const CAENDigitizerModel model,
			const CAEN_DGTZ_ConnectionType& ct, const int& ln, const int& cn,
			const uint32_t& addr, const int& h,
			const CAENError& err) :
			Model(model),
			ConnectionType(ct), LinkNum(ln), ConetNode(cn),
			VMEBaseAddress(addr), Handle(h), LatestError(err) {

			ModelConstants = CAENDigitizerModelsConstants_map.at(model);

		}

		~caen() {
			CAEN_DGTZ_FreeReadoutBuffer(&Data.Buffer);
			Data.Buffer = nullptr;
		}

		// No copying or moving
		caen(caen&&) = delete;
		caen(const caen&) = delete;

		// Public members
		CAENDigitizerModel Model;
		CAENGlobalConfig GlobalConfig;
		std::map<uint8_t, CAENChannelConfig> ChannelConfigs;
		uint32_t CurrentMaxBuffers;

		CAEN_DGTZ_ConnectionType ConnectionType;
		int LinkNum;
		int ConetNode;
		uint32_t VMEBaseAddress;

		int Handle;
		CAENError LatestError;

		// This holds the latest raw CAEN data
		CAENData Data;

		double GetSampleRate() const {
			return ModelConstants.AcquisitionRate;
		}

		uint32_t GetMemoryPerChannel() const {
			return ModelConstants.MemoryPerChannel;
		}

		uint32_t GetMaxNumberOfBuffers() const {
			return ModelConstants.MaxNumBuffers;
		}

		uint32_t GetNumChannels() const {
			return ModelConstants.NumChannels;
		}

		uint32_t GetCommTransferRate() const {
			return ModelConstants.USBTransferRate;
		}

		double GetNLOCTORecordLength() const {
			return ModelConstants.NLOCToRecordLength;
		}

		// Returns the channel voltage range. If channel does not exist
		// returns 0
		double GetVoltageRange(int ch) const {
			try {
				auto config = ChannelConfigs.at(ch);
				return ModelConstants.VoltageRanges[config.DCRange];
			} catch(...) {
				return 0.0;
			}
			return 0.0;
		}

private:
		CAENDigitizerModelConstants ModelConstants;
	};

	using CAEN = std::unique_ptr<caen>;

	template<typename PrintFunc>
	// Check if there is an error and prints it out using the lambda/function f
	// it also clears the error
	bool check_error(CAEN& res, PrintFunc&& f) noexcept {
		if(!res) {
			f("There is no resource to check error, maybe that is the"
				"problem?");
			return true;
		}

		if(res->LatestError.isError || res->LatestError.ErrorCode < 0) {
			auto error_str =
				std::to_string(static_cast<int>(res->LatestError.ErrorCode));
			f("Error code: " + error_str);
			f(res->LatestError.ErrorMessage);

			// res->LatestError.ErrorMessage = "";
			// res->LatestError.ErrorCode = CAEN_DGTZ_ErrorCode::CAEN_DGTZ_Success;
			// res->LatestError.isError = false;
			return true;
		}

		return false;
	}

	template<typename PrintFunc>
	// Check if there is an error and prints it out using the lambda/function f
	// it also clears the error
	bool check_error(const CAENError& err, PrintFunc&& f) noexcept {

		if(err.isError || err.ErrorCode < 0) {
			auto error_str =
				std::to_string(static_cast<int>(err.ErrorCode));
			f("Error code: " + error_str);
			f(err.ErrorMessage);

			return true;
		}

		return false;
	}

	// Opens the resource and returns a handle if successful.
	// ct -> Physical communication channel. CAEN_DGTZ_USB,
	//  CAEN_DGTZ_PCI_OpticalLink being the most expected options
	// ln -> Link number. In the case of USB is the link number assigned by the
	//  PC when you connect the cable to the device. 0 for the first device and
	//  so on. There is no fix correspondence between the USB port and the
	//  link number.
	//  For CONET, indicates which link of A2818 or A3818 is used.
	// cn -> Conet Node. Identifies which device in the daisy chain is being
	//  addressed.
	// addr -> VME Base adress of the board. Only for VME models.
	//  0 in all other cases
	CAENError connect(CAEN&, const CAENDigitizerModel&,
		const CAEN_DGTZ_ConnectionType&, const int&,
		const int&, const uint32_t&) noexcept;

	// Simplifies connect() func for the use of USB only.
	CAENError connect_usb(CAEN&, const CAENDigitizerModel&, const int&) noexcept;

	// Releases the CAEN resource, stop acquisition, clear internal buffer,
	// and closes the communication with the CAEN digitizer
	CAENError disconnect(CAEN&) noexcept;

	// Reset. Returns all internal registers to defaults.
	// Does not reset if resource is null.
	void reset(CAEN&) noexcept;

	// Calibrates the device only after the temperature has stabilized.
	// NOT YET IMPLEMENTED
	// Does not calibrate if resource is null or there are errors
	void calibrate(CAEN&) noexcept;

	// Calls a bunch of setup functions for each channel configuration
	// found under the vector<CAENChannelConfigs> in res
	// Does not setup if resource is null or there are errors.
	void setup(CAEN&, CAENGlobalConfig, std::vector<CAENChannelConfig>) noexcept;

	// Enables the acquisition and allocates the memory for the acquired data.
	// Does not enable acquisitoin if resource is null or there are errors.
	void enable_acquisition(CAEN&) noexcept;

	// Disables the acquisition and frees up memory
	// Does not disables acquisition if resource is null.
	void disable_acquisition(CAEN&) noexcept;

	// Writes to register ADDR with VALUE
	// Does write to register if resource is null or there are errors.
	void write_register(CAEN&,
		uint32_t&& addr,
		const uint32_t& value) noexcept;

	// Reads contents of register ADDR into value
	// Does not modify value if resource is null or there are errors.
	void read_register(CAEN&,
		uint32_t&& addr,
		uint32_t& value) noexcept;

	// Forces a software trigger in the digitizer.
	// Does not trigger if resource is null or there are errors.
	void software_trigger(CAEN&) noexcept;

	// Asks CAEN how many events are in the buffer
	// Returns 0 if resource is null or there are errors.
	uint32_t get_events_in_buffer(CAEN&) noexcept;

	// Runs a bunch of commands to retrieve the buffer and process it
	// using CAEN functions.
	// Does not retrieve data if resource is null or there are errors.
	void retrieve_data(CAEN&) noexcept;

	// Returns true if data was read successfully
	// Does not retrieve data if resource is null, there are errors,
	// or events in buffer are less than n.
	// n cannot be bigger than the max number of buffers allowed
	bool retrieve_data_until_n_events(CAEN&, uint32_t&& n) noexcept;

	// Extracts event i from the data retrieved by retrieve_data(...)
	// into Event evt.
	// If evt == NULL it allocates memory, slower
	// evt can be allocated using allocate_event(...) before hand.
	// If i is beyond the NumEvents, it does nothing.
	// Does not retrieve data if resource is null or there are errors.
	void extract_event(CAEN&, const uint32_t& i, CAENEvent& evt) noexcept;

	// Clears the digitizer buffer.
	// Does not do any error checking. Do not pass a null port!!
	// This is to optimize for speed.
	void clear_data(CAEN&) noexcept;

	// From CAEN:
	// Retrieves how many times per second the input pulse crossed the
	// programmed threshold of the channel n digital discriminator.
	// Only for 725S 730S models
	// uint32_t channel_self_trigger_meter(CAEN&, uint8_t channel) noexcept;


	// Mathematical functions

	// Turns a time (in nanoseconds) into counts
	// Reminder that some digitizers take data in chunks
	// so some record lengths are not possible.
	// Ex: x5730 record length can only be multiples of 4
	uint32_t t_to_record_length(CAEN&, double) noexcept;

	// Turns a voltage (V) into trigger counts
	uint32_t v_to_threshold_counts(CAEN&, double) noexcept;

	// Turns a voltage (V) into count offset
	uint32_t v_offset_to_count_offset(CAEN&, double) noexcept;

	// Calculates the max number of buffers for a given record length
	uint32_t calculate_max_buffers(CAEN&) noexcept;

	std::string sbc_init_file(CAEN&) noexcept;
	std::string sbc_save_func(CAENEvent& evt, CAEN& res) noexcept;

} // namespace SBCQueens