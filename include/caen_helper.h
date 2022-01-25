#pragma once

// std includes
#include "CAENComm.h"
#include <cstdint>
#include <memory>
#include <string>

// 3rd party includes
#include <CAENDigitizer.h>

// my includes

namespace SBCQueens {

	struct ChannelConfig {
		uint8_t Channel = 0;

		// 0x8000 no offset
		uint32_t DCOffset = 0x8000;
		// 250 = 10us
		uint32_t RecordLength = 2500;
		// 0 = 2Vpp? 1 = 0.5Vpp?
		uint8_t DCRange = 0;

		// In ADC counts
		uint32_t TriggerThreshold = 0;
		CAEN_DGTZ_TriggerMode_t TriggerMode 
			= CAEN_DGTZ_TriggerMode_t::CAEN_DGTZ_TRGMODE_ACQ_ONLY;

		// DT5730 max buffers is 1024
		uint32_t MaxEventsPerRead = 1024;
		CAEN_DGTZ_AcqMode_t AcqMode 
			= CAEN_DGTZ_AcqMode_t::CAEN_DGTZ_SW_CONTROLLED;

		// True = enabled, False = disabled
		bool TriggerOverlappingEn = true;
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
	// and the pointer to the point in the original buffer.
	struct Event {
		char* DataPtr;
		CAEN_DGTZ_UINT16_EVENT_t* Data = nullptr;
		CAEN_DGTZ_EventInfo_t Info;
	};

	struct CAEN {

		CAEN() {}
		CAEN(const CAEN_DGTZ_ConnectionType& ct, const int& ln, const int& cn, 
			const uint32_t& addr, std::unique_ptr<int>& h,
			const CAEN_DGTZ_ErrorCode& err) :
		ConnectionType(ct), LinkNum(ln), ConetNode(cn), VMEBaseAddress(addr),
		Handle(std::move(h)), LatestError(err) {

			ReadData = std::make_shared<CAENData>(Data);
			WriteData = std::make_shared<CAENData>(BackupData);
		}

		~CAEN() {
			CAEN_DGTZ_FreeReadoutBuffer(&Data.Buffer);
			CAEN_DGTZ_FreeReadoutBuffer(&BackupData.Buffer);
			Data.Buffer = nullptr;
			BackupData.Buffer = nullptr;
		}

		// Only move
		CAEN(CAEN&&) = default;
		CAEN& operator=(CAEN&&) = default;

		// No copying
		CAEN(const CAEN&) = delete;

		CAEN_DGTZ_ConnectionType ConnectionType;
		int LinkNum;
		int ConetNode;
		uint32_t VMEBaseAddress;

		std::unique_ptr<int> Handle;

		CAENData Data;
		CAENData BackupData;

		std::shared_ptr<CAENData> ReadData;
		std::shared_ptr<CAENData> WriteData;

		CAEN_DGTZ_ErrorCode LatestError;

	};

	using CAEN_ptr = std::unique_ptr<CAEN>;

	std::string translate_caen_code(const CAEN_DGTZ_ErrorCode&) noexcept;

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
	bool connect(CAEN_ptr&, const CAEN_DGTZ_ConnectionType&, const int&,
		const int&, const uint32_t&) noexcept;

	// Simplifies connect() func for the use of USB only.
	bool connect_usb(CAEN_ptr&, const int&) noexcept;

	// Releases the resources and closes the communication
	// with the CAEN digitizer
	bool disconnect(CAEN_ptr&) noexcept;

	// Reset. Returns all internal registers to defaults
	bool reset(CAEN_ptr&) noexcept;

	// Calibrates the device only after the temperature has stabilized.
	// NOT YET IMPLEMENTED
	bool calibrate(CAEN_ptr&) noexcept;

	// Calls a bunch of setup functions for each channel passed down in the
	// list configs
	bool setup(CAEN_ptr&,
		const std::initializer_list<ChannelConfig>& configs) noexcept;

	// Enables the acquisition and allocates the memory for the acquired data.
	bool enable_acquisition(CAEN_ptr&) noexcept;

	// Disables the acquisition
	bool disable_acquisition(CAEN_ptr&) noexcept;

	// Writes to registers in ADDR with VALUE
	bool write_register(CAEN_ptr&,
		uint32_t&& addr,
		const uint32_t& value) noexcept;

	// Reads contents of register ADDR into register value
	bool read_register(CAEN_ptr&,
		uint32_t&& addr,
		uint32_t& value) noexcept;

	// Forces a trigger in the digitizer.
	bool software_trigger(CAEN_ptr&) noexcept;

	// Runs a bunch of commands to retrieve the buffer and process it
	// using CAEN functions.
	// Does not do any error checking. Do not pass a null port!!
	// This is to optimize for speed.
	void retrieve_data(CAEN_ptr&) noexcept;

	// Extracts event i from the data retrieved by retrieve_data(...)
	// If i is beyond the NumEvents, it does nothing and returns an empty event.
	// If an error happened, output event is empty, this is to allow concurrency.
	Event extract_event(CAEN_ptr&, const uint32_t& i) noexcept;

	// Extracts event i from the data retrieved by retrieve_data(...)
	// into Event evt
	// If i is beyond the NumEvents, it does nothing
	// If an error happened, it does nothing this is to allow concurrency.
	// Event must have been allocated beforehand, if not this will not work.
	// NO ERROR CHECKING. Optimized for speed
	void extract_event(CAEN_ptr&, const uint32_t& i, Event& evt) noexcept;

	// Allocates event for using with extract_event(...)
	void allocate_event(CAEN_ptr&, Event&) noexcept;

	// Frees memory from an event created by extract_event(...)
	void free_event(CAEN_ptr&, Event&) noexcept;

	// Clears the digitizer buffer.
	// Does not do any error checking. Do not pass a null port!!
	// This is to optimize for speed.
	void clear_data(CAEN_ptr&) noexcept;

	// Check if the port has any error
	bool check_for_error(CAEN_ptr&) noexcept;

} // namespace SBCQueens