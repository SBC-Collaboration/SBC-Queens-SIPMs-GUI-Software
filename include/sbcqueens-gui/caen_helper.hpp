#ifndef CAENHELPER_H
#define CAENHELPER_H
#include <numeric>
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <cstdint>
#include <cwchar>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <cmath>
#include <chrono>
#include <array>
#include <variant>

// C++ 3rd party includes
#include <CAENComm.h>
#include <CAENDigitizer.h>

// my includes

namespace SBCQueens {

enum class CAENDigitizerFamilies {
    DEBUG = 0,
    x721, x724, x725,
    x731,
    x730, // Actually supported
    x740, // Actually supported
    x742, x743,
    x751,
    x761,
    x780, x781, x782,
    x790
};

enum class CAENConnectionType {
    USB,
    A4818,
    OpticalLink, // Not supported
    Ethernet_V4718, // Only available for V4718 (not supported)
    USB_V4718 // not supported
};

enum class CAENDigitizerModel {
    DEBUG = 0,
    DT5730B,
    DT5740D,
    V1740D
};

// All the constants that never change for a given digitizer
struct CAENDigitizerModelConstants {
    // ADC resolution in bits
    uint32_t ADCResolution = 8;
    // In S/s
    double AcquisitionRate = 10e6;
    // In S/ch
    uint32_t MemoryPerChannel = 0;
    // Total number of channels
    uint8_t NumChannels = 1;
    // If 0, digitizer does not deal in groups
    uint8_t NumberOfGroups = 0;
    // Number of channels per group
    uint8_t NumChannelsPerGroup = 1;

    uint32_t MaxNumBuffers = 1024;

    float NLOCToRecordLength = 1;

    std::vector<double> VoltageRanges = {};
};

// This is here so we can transform string to enums
const static inline std::unordered_map<std::string, CAENDigitizerModel>
    CAENDigitizerModelsMap {
        {"DEBUG", CAENDigitizerModel::DEBUG},
        {"DT5730B", CAENDigitizerModel::DT5730B},
        {"DT5740D", CAENDigitizerModel::DT5740D},
        {"V1740D", CAENDigitizerModel::V1740D}
};

// These links all the enums with their constants or properties that
// are fixed per digitizer
const static inline std::unordered_map<CAENDigitizerModel, CAENDigitizerModelConstants>
    CAENDigitizerModelsConstantsMap {
        {CAENDigitizerModel::DEBUG, CAENDigitizerModelConstants {
            8,          // ADCResolution
            100e3,      //  AcquisitionRate
            1024ul,     // MemoryPerChannel
            1,          // NumChannels
            0,          // NumberOfGroups, 0 -> no groups
            1,          // NumChannelsPerGroup
            1024,       // MaxNumBuffers
            10.0f,      // NLOCToRecordLength
            {1.0}       // VoltageRanges
        }},
        {CAENDigitizerModel::DT5730B, CAENDigitizerModelConstants {
            14,         // ADCResolution
            500e6,      //  AcquisitionRate
            static_cast<uint32_t>(5.12e6),  // MemoryPerChannel
            8,          // NumChannels
            0,          // NumberOfGroups, 0 -> no groups
            8,          // NumChannelsPerGroup
            1024,       // MaxNumBuffers
            10.0f,      // NLOCToRecordLength
            {0.5, 2.0}  // VoltageRanges
        }},
        {CAENDigitizerModel::DT5740D, CAENDigitizerModelConstants {
            12,         // ADCResolution
            62.5e6,     //  AcquisitionRate
            static_cast<uint32_t>(192e3),  // MemoryPerChannel
            32,         // NumChannels
            4,          // NumChannelsPerGroup
            8,          // NumberOfGroups
            1024,       // MaxNumBuffers
            1.5f,       // NLOCToRecordLength
            {2.0, 10.0} // VoltageRanges
        }},
        {CAENDigitizerModel::V1740D, CAENDigitizerModelConstants {
            12,         // ADCResolution
            62.5e6,     //  AcquisitionRate
            static_cast<uint32_t>(192e3),  // MemoryPerChannel
            64,         // NumChannels
            8,          // NumChannelsPerGroup
            8,          // NumberOfGroups
            1024,       // MaxNumBuffers
            1.5f,       // NLOCToRecordLength
            {2.0}       // VoltageRanges
        }}
    // This is a C++20 higher feature so lets keep everything 17 compliant
    // CAENDigitizerModelsConstants_map {
    //     {CAENDigitizerModel::DT5730B, CAENDigitizerModelConstants{
    //         .ADCResolution = 14,
    //         .AcquisitionRate = 500e6,
    //         .MemoryPerChannel = static_cast<uint32_t>(5.12e6),
    //         .NumChannels = 8,
    //         .NumberOfGroups = 0,
    //         .NumChannelsPerGroup = 8,
    //         .NLOCToRecordLength = 10,
    //         .VoltageRanges = {0.5, 2.0}
    //     }},
    //     {CAENDigitizerModel::DT5740D, CAENDigitizerModelConstants{
    //         .ADCResolution = 12,
    //         .AcquisitionRate = 62.5e6,
    //         .MemoryPerChannel = static_cast<uint32_t>(192e3),
    //         .NumChannels = 32,
    //         .NumberOfGroups = 4,
    //         .NumChannelsPerGroup = 8,
    //         .NLOCToRecordLength = 1.5,
    //         .VoltageRanges = {2.0, 10.0}
    //     }}
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
    uint32_t RecordLength = 100;

    // In %
    uint32_t PostTriggerPorcentage = 50;

    // If enabled, the trigger acquisition only happens whenever
    // EXT or TRG-IN is high.
    bool EXTasGate = false;
    // see caen_helper.cpp setup(...) to see a description of these enums
    CAEN_DGTZ_TriggerMode_t EXTTriggerMode
        = CAEN_DGTZ_TriggerMode_t::CAEN_DGTZ_TRGMODE_ACQ_ONLY;

    CAEN_DGTZ_TriggerMode_t SWTriggerMode
        = CAEN_DGTZ_TriggerMode_t::CAEN_DGTZ_TRGMODE_ACQ_ONLY;

    CAEN_DGTZ_TriggerMode_t CHTriggerMode
        = CAEN_DGTZ_TriggerMode_t::CAEN_DGTZ_TRGMODE_ACQ_ONLY;

    CAEN_DGTZ_AcqMode_t AcqMode
        = CAEN_DGTZ_AcqMode_t::CAEN_DGTZ_SW_CONTROLLED;

    CAEN_DGTZ_IOLevel_t IOLevel
        = CAEN_DGTZ_IOLevel_t::CAEN_DGTZ_IOLevel_NIM;

    // This feature is for x730, x740
    // True = disabled, False = enabled
    bool TriggerOverlappingEn = false;

    // 0 = normal, the board is full whenever all buffers are full
    // 1 = One buffer free. The board is full whenever Nb-1 buffers
    // are full, where Nb is the overall number of numbers of buffers
    // in which the channel memory is divided.
    bool MemoryFullModeSelection = true;

    // 0L = Rising edge, 1L = Falling edge
    CAEN_DGTZ_TriggerPolarity_t TriggerPolarity =
        CAEN_DGTZ_TriggerPolarity_t::CAEN_DGTZ_TriggerOnRisingEdge;
};

// Help structure to link an array of booleans to a single uint8_t
// without the use of other C++ features which makes it trickier to use
struct ChannelsMask {
    std::array<bool, 8> CH
        = {false, false, false, false, false, false, false, false};

    uint8_t get() noexcept {
        uint8_t out = 0u;
        for(std::size_t i = 0; i < CH.size(); i++) {
            out |= static_cast<uint8_t>(CH[i]) << i;
        }
        return out;
    }

    bool& operator[](const std::size_t& iter) noexcept {
        return CH[iter];
    }
};

// As a general case, this holds all the configuration values for a channel
// if a digitizer does not support groups, i.e x730, group = channel
struct CAENGroupConfig {
    // Is this group/channel enabled?
    bool Enabled = 0;

    // Mask of channels within the group enabled to trigger
    // If Channel, if its != 0 then its enabled
    ChannelsMask TriggerMask;

    // Mask of enabled channels within the group
    // Ignored for single channels.
    ChannelsMask AcquisitionMask;

    // DC offsets of each channel in the group in ADC counts.
    // Usually the DC offset is a 16bit DAC. Check documentation.
    uint32_t DCOffset = 0x8000;
    std::array<uint8_t, 8> DCCorrections = {0, 0, 0, 0, 0, 0, 0, 0};

    // For digitizers that support several ranges.
    // 0 = 2Vpp, 1 = 0.5Vpp
    // DC range of the group or channel
    uint8_t DCRange = 0;

    // In ADC counts
    uint32_t TriggerThreshold = 0;
};

// Holds the CAEN raw data, the size of the buffer, the size
// of the data and number of events
// pointer Buffer is meant to be allocated using CAEN functions
// after a setup(...) call.
struct CAENData {
    char* Buffer = nullptr;
    uint32_t TotalSizeBuffer;
    uint32_t DataSize;
    uint32_t NumEvents;
};

// Events structure: holds the raw data of the event, the info (timestamp),
// and the pointer to the point in the original buffer.
// This uses CAEN functions to allocate memory, so if handle does not
// have an associated digitizer to it, it will not work.
// Create events only after a setup(...) call has been made for best
// results.
struct caenEvent {
    // Treat this like a shared pointer.
    char* DataPtr = nullptr;
    // Treat this like an unique pointer
    CAEN_DGTZ_UINT16_EVENT_t* Data = nullptr;
    CAEN_DGTZ_EventInfo_t Info;

    explicit caenEvent(const int& handle)
        : _handle(handle) {
        // This will only allocate memory if handle does
        // has an associated digitizer to it.
        CAEN_DGTZ_AllocateEvent(_handle, reinterpret_cast<void**>(&Data));
    }

    // If handle is released before this event is freed,
    // it will cause a memory leak, maybe?
    ~caenEvent() {
        CAEN_DGTZ_FreeEvent(_handle, reinterpret_cast<void**>(&Data) );
    }

    // // This copies event other into this
    // caenEvent(const caenEvent& other) {
    //     this->_handle = other._handle;
    //     this->DataPtr = other.DataPtr;
    //     CAEN_DGTZ_AllocateEvent(other._handle,
    //         reinterpret_cast<void**>(&this->Data));
    //     // *A = *B copies the data
    //     *this->Data = *other.Data;
    //     this->Info = other.Info;
    // }

    // caenEvent operator=(const caenEvent& other) {
    //     return caenEvent(other);
    // }

 private:
    int _handle;
};

using CAENEvent = std::shared_ptr<caenEvent>;

// The main CAEN struct. Holds the model, all its parameters
// and the raw binary data from the digitizer.
struct caen {
    caen(const CAENDigitizerModel model,
        const CAENConnectionType& ct, const int& ln, const int& cn,
        const uint32_t& addr, const int& h,
        const CAENError& err) :
        Model(model),
        ConnectionType(ct), LinkNum(ln), ConetNode(cn),
        VMEBaseAddress(addr), Handle(h), LatestError(err),
        ModelConstants(CAENDigitizerModelsConstantsMap.at(model)) {

        if (model == CAENDigitizerModel::DT5730B) {
            Family = CAENDigitizerFamilies::x730;
        } else if (model == CAENDigitizerModel::DT5740D ||
            model == CAENDigitizerModel::V1740D) {
            Family = CAENDigitizerFamilies::x740;
        }

    }

    ~caen() {
        // Moved the deallocation to disconnect(...)
        // CAEN_DGTZ_FreeReadoutBuffer(&Data.Buffer);
        Data.Buffer = nullptr;
    }

    // No copying or moving
    caen(caen&&) = delete;
    caen(const caen&) = delete;

    // Public members
    CAENDigitizerFamilies Family;
    CAENDigitizerModel Model;
    CAEN_DGTZ_BoardInfo_t CAENBoardInfo;
    CAENGlobalConfig GlobalConfig;
    std::array<CAENGroupConfig, 8> GroupConfigs;
    uint32_t CurrentMaxBuffers;

    CAENConnectionType ConnectionType;
    int LinkNum;
    int ConetNode;
    uint32_t VMEBaseAddress;

    int Handle;
    CAENError LatestError;
    const CAENDigitizerModelConstants ModelConstants;

    bool start_rate_calculation = false;
    std::chrono::high_resolution_clock::time_point ts, te;
    uint64_t t_us, n_events;
    uint64_t trg_count = 0, duration = 0;

    // This holds the latest raw CAEN data
    CAENData Data;

    uint32_t GetCommTransferRate() const {
        if(ConnectionType == CAENConnectionType::USB){
            return 15000000u;  // S/s
        } else if (ConnectionType == CAENConnectionType::A4818) {
            return 40000000u;
        }

        return 0u;
    }

    // Returns the channel voltage range. If channel does not exist
    // returns 0
    double GetVoltageRange(const uint8_t& gr_n) const {
        try {
            auto config = GroupConfigs[gr_n];
            return ModelConstants.VoltageRanges[config.DCRange];
        } catch(...) {
            return 0.0;
        }
        return 0.0;
    }
};

using CAEN = std::unique_ptr<caen>;

/// Error checking functions

template<typename PrintFunc>
// Check if there is an error and prints it out using the lambda/function f
// it also clears the error
bool check_error(CAEN& res, PrintFunc&& f) noexcept {
    if (!res) {
        f("There is no resource to check error, maybe that is the"
            "problem?");
        return true;
    }

    if (res->LatestError.isError || res->LatestError.ErrorCode < 0) {
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
    if (err.isError || err.ErrorCode < 0) {
        auto error_str =
            std::to_string(static_cast<int>(err.ErrorCode));
        f("Error code: " + error_str);
        f(err.ErrorMessage);

        return true;
    }

    return false;
}

/// End Error checking functions
//
/// General CAEN functions

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
// addr -> VME Base address of the board. Only for VME models.
//  0 in all other cases
CAENError connect(CAEN&, const CAENDigitizerModel&,
    const CAENConnectionType&, const int&,
    const int&, const uint32_t&) noexcept;

// Simplifies connect() func for the use of USB only.
// CAENError connect_usb(CAEN&, const CAENDigitizerModel&, const int&) noexcept;

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
void setup(CAEN&, CAENGlobalConfig,
           const std::array<CAENGroupConfig, 8>&) noexcept;

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

// Write arbitrary bits of any length at any position,
// Keeps the other bits unchanged,
// Following instructions at https://stackoverflow.com/questions/11815894/how-to-read-write-arbitrary-bits-in-c-c
void write_bits(CAEN& res, uint32_t&& addr,
    const uint32_t& value, uint8_t pos, uint8_t len = 1) noexcept;

// Forces a software trigger in the digitizer.
// Does not trigger if resource is null or there are errors.
void software_trigger(CAEN&) noexcept;

/// End General CAEN functions
//
/// Data Acquisition functions

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
bool retrieve_data_until_n_events(CAEN&, uint32_t& n,
    bool debug_info = false) noexcept;

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

/// End Data Acquisition functions
//
/// Mathematical functions

// Turns a time (in nanoseconds) into counts
// Reminder that some digitizers take data in chunks or only allow
// some record lengths, so some record lengths are not possible.
// Ex: x5730 record length can only be multiples of 10
uint32_t t_to_record_length(CAEN&, const double&) noexcept;

// Turns a voltage threshold counts to ADC cts
uint32_t v_threshold_cts_to_adc_cts(const CAEN&, const uint32_t&) noexcept;

// Turns a voltage (V) into trigger counts
uint32_t v_to_threshold_counts(const double&) noexcept;

// Turns a voltage (V) into count offset
uint32_t v_offset_to_count_offset(const CAEN&, const double&) noexcept;

// Calculates the max number of buffers for a given record length
uint32_t calculate_max_buffers(CAEN&) noexcept;

/// End Mathematical functions
//
/// File functions

// Saves the digitizer data in the Binary format SBC collboration is using
// This only writes the header at the beginning of the file.
// Meant to be written once.
std::string sbc_init_file(CAEN&) noexcept;

// Saves the digitizer data in the Binary format SBC collboration is using
std::string sbc_save_func(CAENEvent& evt, CAEN& res) noexcept;

/// End File functions

}   // namespace SBCQueens
#endif
