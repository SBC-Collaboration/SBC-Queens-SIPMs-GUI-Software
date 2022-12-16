#ifndef CAENHELPER_H
#define CAENHELPER_H
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

// C++ 3rd party includes
#include <CAENComm.h>
#include <CAENDigitizer.h>

// my includes
#include "logger_helpers.hpp"

namespace SBCQueens {

std::string translate_caen_error_code(const CAEN_DGTZ_ErrorCode& err);

// This list is incomplete
enum class CAENConnectionType {
    USB,
    A4818
};

// This list is incomplete
enum class CAENDigitizerModel {
    DT5730B = 0,
    DT5740D = 1
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

    // Constant to transform Nloc to Recordlength
    float NLOCToRecordLength = 1;

    std::vector<double> VoltageRanges;
};

// This is here so we can transform string to enums
// If it the key does not exist, this will throw an error
const std::unordered_map<std::string, CAENDigitizerModel>
    CAENDigitizerModels_map {
        {"DT5730B", CAENDigitizerModel::DT5730B},
        {"DT5740D", CAENDigitizerModel::DT5740D}
};

// These maps are here to link all the enums with their constants or properties
// that are fixed per digitizer. They can only be fixed by hand.
const std::unordered_map<CAENDigitizerModel, CAENDigitizerModelConstants>
    CAENDigitizerModelsConstants_map {
        {CAENDigitizerModel::DT5730B,
        CAENDigitizerModelConstants {
            14,  // ADCResolution
            500e6,  //  AcquisitionRate
            static_cast<uint32_t>(5.12e6),  // MemoryPerChannel
            8,  // NumChannels
            0,  // NumberOfGroups
            8,  // NumChannelsPerGroup
            1024,  // MaxNumBuffers
            10.0f,  // NLOCToRecordLength
            {0.5, 2.0}  // VoltageRanges
        }},
        {CAENDigitizerModel::DT5740D, CAENDigitizerModelConstants {
             12,  // ADCResolution
             62.5e6,  //  AcquisitionRate
             static_cast<uint32_t>(192e3),  // MemoryPerChannel
             32,  // NumChannels
             4,  // NumChannels
             8,  // NumberOfGroups
             1024,  // MaxNumBuffers
             1.5f,  // NLOCToRecordLength
             {2.0, 10.0}  // VoltageRanges
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

struct CAENGlobalConfig {
    // X730 max buffers is 1024
    // X740 max buffers is 1024
    uint32_t MaxEventsPerRead = 512;

    // Record length in samples
    uint32_t RecordLength = 400;

    // In %
    uint32_t PostTriggerPorcentage = 50;

    // If enabled, the trigger acquisition only happens whenever
    // EXT or TRG-IN is high.
    bool EXTAsGate = false;

    // External Trigger Mode
    // see caen_helper.cpp setup(...) to see a description of these enums
    CAEN_DGTZ_TriggerMode_t EXTTriggerMode
        = CAEN_DGTZ_TriggerMode_t::CAEN_DGTZ_TRGMODE_ACQ_ONLY;

    // Software Trigger Mode
    // see caen_helper.cpp setup(...) to see a description of these enums
    CAEN_DGTZ_TriggerMode_t SWTriggerMode
        = CAEN_DGTZ_TriggerMode_t::CAEN_DGTZ_TRGMODE_ACQ_ONLY;

    // Channel Trigger Mode
    // see caen_helper.cpp setup(...) to see a description of these enums
    CAEN_DGTZ_TriggerMode_t CHTriggerMode
        = CAEN_DGTZ_TriggerMode_t::CAEN_DGTZ_TRGMODE_ACQ_ONLY;

    // Acquisition mode
    CAEN_DGTZ_AcqMode_t AcqMode
        = CAEN_DGTZ_AcqMode_t::CAEN_DGTZ_SW_CONTROLLED;

    // Voltage i/o level for the digital channels
    CAEN_DGTZ_IOLevel_t IOLevel
        = CAEN_DGTZ_IOLevel_t::CAEN_DGTZ_IOLevel_NIM;

    // This feature is only available for X730
    // True = disabled, False = enabled
    bool TriggerOverlappingEn = false;

    // false -> normal, the board is full whenever all buffers are full.
    // true  -> always one buffer free. The board is full whenever Nb-1 buffers
    // are full, where Nb is the overall number of numbers of buffers allocated.
    bool MemoryFullModeSelection = true;

    // Triggering edge option
    // 0L/CAEN_DGTZ_TriggerOnRisingEdge -> Rising edge,
    // 1L/CAEN_DGTZ_TriggerOnFallingEdge -> Falling edge
    CAEN_DGTZ_TriggerPolarity_t TriggerPolarity =
        CAEN_DGTZ_TriggerPolarity_t::CAEN_DGTZ_TriggerOnRisingEdge;
};

// As a general case, this holds all the configuration values for a channel
// if a digitizer does not support groups, i.e x730, group = channel
struct CAENGroupConfig {
    // channel # or channel group
    uint8_t Number = 0;

    // Mask of channels within the group enabled to trigger
    // If Channel, if its != 0 then its enabled
    uint8_t TriggerMask = 0;

    // Mask of enabled channels within the group
    // Ignored for single channels.
    uint8_t AcquisitionMask = 0;

    // 0x8000 no offset if 16 bit DAC
    // 0x1000 no offset if 14 bit DAC
    // 0x400 no offset if 14 bit DAC
    // DC offsets of each channel in the group
    uint16_t DCOffset;
    std::vector<uint8_t> DCCorrections;

    // For DT5730
    // 0 = 2Vpp, 1 = 0.5Vpp
    // DC range of the group or channel
    uint8_t DCRange = 0;

    // In ADC counts
    uint16_t TriggerThreshold = 0;
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

    // This copies event other into this
    caenEvent(const caenEvent& other) {
        this->_handle = other._handle;
        this->DataPtr = other.DataPtr;
        CAEN_DGTZ_AllocateEvent(other._handle,
            reinterpret_cast<void**>(&this->Data));
        // *A = *B copies the data
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

template<typename Logger = std::shared_ptr<iostream_wrapper>>
struct CAEN {
 private:

    // Holds the CAEN raw data, the size of the buffer, the size
    // of the data and number of events
    // pointer Buffer is meant to be allocated using CAEN functions
    // after a setup(...) call.
    class CAENData {
     private:
        void _clear_memory(char* p) {
            CAEN_DGTZ_FreeReadoutBuffer(&p);
        }
     public:
        std::unique_ptr<char[],
            decltype(&CAENData::_clear_memory)> Buffer;
        uint32_t TotalSizeBuffer = 0;
        uint32_t DataSize = 0;
        uint32_t NumEvents = 0;

        CAENData() : Buffer(new char, &CAENData::_clear_memory) { }
    };

    static inline std::unordered_map<int, bool> _connection_info_map;

    // THis is a number that the CAEN API uses to manage its own resources.
    int _caen_api_handle = -1;
    bool _is_connected = false;
    CAEN_DGTZ_ErrorCode _err_code;
    bool _has_error = false;
    bool _has_warning = false;

    // This holds the latest raw CAEN data
    CAENData _caen_raw_data;

    // Communicated with the outside world: errors, warnings and debug msgs
    // Assumes it is a pointer of any form and this class does not manage
    // its deletion
    Logger& _logger;

    CAENGlobalConfig _global_config;
    std::map<uint8_t, CAENGroupConfig> _group_configs;
    uint32_t _current_max_buffers;

    // Translates the connection info data to a single number that should*
    // be unique.
    constexpr uint64_t _hash_connection_info(const CAENConnectionType& ct,
        const int& ln, const int& cn, const uint32_t& addr) {
        uint64_t id = static_cast<uint64_t>(addr & 0x0000FFFF) << 32;
        id |= (cn & 0x000000FF) << 16;
        id |= (ln & 0x000000FF) << 8;
        switch (ct) {
        case CAENConnectionType::A4818:
            id |= 1;
        case CAENConnectionType::USB:
        default:
            id |= 0;
        }

        return id;
    }

    // Private helper function to wrap the logic behind checking for an error
    // and printing the error message
    template<typename ...Args>
    void _print_if_err(const std::string& msg, Args&&... args) noexcept {
        switch (_err_code) {
        case CAEN_DGTZ_ErrorCode::CAEN_DGTZ_Success:
            break;
        default:
            _has_error = true;
            _logger.error("CAEN ERR = "
                + translate_caen_error_code(_err_code) + "."
                + msg, std::forward<Args...>(args...));
        }
    }

 public:
    // Model
    CAENDigitizerModel Model;
    // Constants associated with the model.
    const CAENDigitizerModelConstants ModelConstants;
    // Type of connection (USB, VME...)
    const CAENConnectionType ConnectionType;
    //
    const int LinkNum;
    //
    const int ConetNode;
    //
    const uint32_t VMEBaseAddress;

    // Creation of this class attempts a connection to CAEN
    // which will take the resource and IsConnected will say if its connected
    // Its up to the user if they would like a more pointer approach
    // or just if-else statements.
    //
    // Description of parameters:
    // logger -> logger used to print information (debug, warning, etc)
    //  defaults iostream_wrapper but can be user configured.
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
    CAEN(Logger& logger, const CAENDigitizerModel model,
        const CAENConnectionType& ct, const int& ln, const int& cn,
        const uint32_t& addr) :
        _logger(logger), Model(model),
        ModelConstants(CAENDigitizerModelsConstants_map.at(model)),
        ConnectionType(ct), LinkNum(ln), ConetNode(cn),
        VMEBaseAddress(addr)
    {
        /// We turn Connection type, linknum, conectnode and VMEBaseAddress
        /// into a single container so we know we are not repeating connections
        auto id = _hash_connection_info(ct, ln, cn, addr);

        // C++17 way to check if the item exists
        // If we move to C++20, we could use _handle_map.contains(...)
        try {
            _connection_info_map.at(id);
            // Past this point it means the connection already exists
            _logger->warn("Resource is already on use. Calling this function "
                "will restart the resource.");
        } catch (...) {
            _connection_info_map[id] = true;
        }

        // If the item exists, pass on the creation
        switch (ct) {
            case CAENConnectionType::A4818:
                _err_code = CAEN_DGTZ_OpenDigitizer(
                    CAEN_DGTZ_ConnectionType::CAEN_DGTZ_USB_A4818,
                    ln, 0, addr, &_caen_api_handle);
            break;

            case CAENConnectionType::USB:
            default:
                _err_code = CAEN_DGTZ_OpenDigitizer(
                    CAEN_DGTZ_ConnectionType::CAEN_DGTZ_USB,
                    ln, cn, 0, &_caen_api_handle);
            break;
        }

        _is_connected = (_err_code < 0) && (_caen_api_handle > 0);
        if (!_is_connected) {
            _caen_api_handle = -1;
            _logger.error(
                "Failed to connect to resource at with CAEN error : {0}",
                translate_caen_error_code(_err_code));
        } else {
            _logger.info("Connected resource with handle {0} with "
                "link number {1}, conet node {2} and VME address {3}.",
                _caen_api_handle, ln, cn, addr);
        }
    }

    ~CAEN() {
        auto id = _hash_connection_info(
            ConnectionType, LinkNum, ConetNode, VMEBaseAddress);
        _connection_info_map.erase(id);

        if (_is_connected) {
            _logger.info("Going to disconnect resource with handle {0} with "
            "link number {1}, conet node {2} and VME address {3}.",
            _caen_api_handle, LinkNum, ConetNode, VMEBaseAddress);

            _err_code = CAEN_DGTZ_SWStopAcquisition(_caen_api_handle);
            _print_if_err("Failed to stop acquisition during cleaning of "
                "resources, what went wrong?");

            _err_code = CAEN_DGTZ_CloseDigitizer(_caen_api_handle);
            _print_if_err("Failed to close digitizer, whaaat?");
        }
        // Everything should be cleared by their own destructors!
    }

    // No copying nor referencing only moving
    CAEN(CAEN&) = delete;
    CAEN(const CAEN&) = delete;

    // Public members
    // Check whenever the port is connected
    bool IsConnected() noexcept { return _is_connected; }

    bool HasError() noexcept { return _has_error; }

    void ResetWarning() { _has_warning = false; }
    bool HasWarning() noexcept { return _has_warning; }

    void Setup(const CAENGlobalConfig&,
        const std::vector<CAENGroupConfig>&) noexcept;
    // Reset. Returns all internal registers to defaults.
    // Does not reset if resource is null.
    void Reset() noexcept;
    // Enables the acquisition and allocates the memory for the acquired data.
    // Does not enable acquisitoin if resource is null or there are errors.
    void EnableAcquisition() noexcept;
    // Disables the acquisition and frees up memory
    // Does not disables acquisition if resource is null.
    void DisableAcquisition() noexcept;// Writes to register ADDR with VALUE
    // Does write to register if resource is null or there are errors.
    void WriteRegister(CAEN&, uint32_t&& addr, const uint32_t& value) noexcept;
    // Reads contents of register ADDR into value
    // Does not modify value if resource is null or there are errors.
    void ReadRegister(uint32_t&& addr, uint32_t& value) noexcept;
    // Write arbitrary bits of any length at any position,
    // Keeps the other bits unchanged,
    // Following instructions at
    // https://stackoverflow.com/questions/11815894/how-to-read-write-arbitrary-bits-in-c-c
    void WriteBits(uint32_t&& addr,
        const uint32_t& value, uint8_t pos, uint8_t len = 1) noexcept;
    // Forces a software trigger in the digitizer.
    // Does not trigger if resource is null or there are errors.
    void SoftwareTrigger() noexcept;
    // Asks CAEN how many events are in the buffer
    // Returns 0 if resource is null or there are errors.
    uint32_t GetEventsInBuffer() noexcept;
    // Runs a bunch of commands to retrieve the buffer and process it
    // using CAEN functions.
    // Does not retrieve data if resource is null or there are errors.
    void RetrieveData() noexcept;
    // Returns true if data was read successfully
    // Does not retrieve data if resource is null, there are errors,
    // or events in buffer are less than n.
    // n cannot be bigger than the max number of buffers allowed
    bool RetrieveDataUntilNEvents(uint32_t& n, bool debug_info = false) noexcept;
    // Extracts event i from the data retrieved by retrieve_data(...)
    // into Event evt.
    // If evt == NULL it allocates memory, slower
    // evt can be allocated using allocate_event(...) before hand.
    // If i is beyond the NumEvents, it does nothing.
    // Does not retrieve data if resource is null or there are errors.
    void ExtractEvent(const uint32_t& i, CAENEvent& evt) noexcept;
    // Clears the digitizer buffer.
    // Does not do any error checking. Do not pass a null port!!
    // This is to optimize for speed.
    void ClearData() noexcept;


    bool start_rate_calculation = false;
    std::chrono::high_resolution_clock::time_point ts, te;
    uint64_t t_us, n_events;
    uint64_t trg_count = 0, duration = 0;

    uint32_t GetCurrentPossibleMaxBuffer() const {
        return _current_max_buffers;
    }

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
    double GetVoltageRange(const uint8_t& ch) const {
        try {
            auto config = _group_configs.at(ch);
            return ModelConstants.VoltageRanges[config.DCRange];
        } catch(...) {
            return 0.0;
        }
        return 0.0;
    }
};


/// General CAEN functions

template<typename T>
void CAEN<T>::Reset() noexcept {
    if (_has_error) {
        return;
    }

    _err_code = CAEN_DGTZ_Reset(_caen_api_handle);
    _print_if_err("Failed to reset the CAEN");
}

template<typename T>
void CAEN<T>::Setup(const CAENGlobalConfig& g_config,
    const std::vector<CAENGroupConfig>& gr_configs) noexcept {
    if (_has_error || !_is_connected) {
        return;
    }

    Reset();

    int &handle = _caen_api_handle;
    int latest_err = 0;
    std::string err_msg = "";

    // This lambda wraps whatever CAEN function you pass to it,
    // checks the error, and if there was an error, add the error msg to
    // it. If there is an error, it also does not execute the function.
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
    //  res->GlobalConfig.MaxEventsPerRead);
    // if(err < 0) {
    //    total_err |= err;
    //    err_msg += "CAEN_DGTZ_SetMaxNumEventsBLT Failed. ";
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
    res->GlobalConfig.RecordLength
        = res->ModelConstants.NLOCToRecordLength * nloc;

    error_wrap("CAEN_DGTZ_SetPostTriggerSize Failed. ",
                         CAEN_DGTZ_SetPostTriggerSize, handle,
                         res->GlobalConfig.PostTriggerPorcentage);

    // Trigger Mode comes in four flavors:
    // CAEN_DGTZ_TRGMODE_DISABLED
    //    is disabled
    // CAEN_DGTZ_TRGMODE_EXTOUT_ONLY
    //    is used to generate the trigger output
    // CAEN_DGTZ_TRGMODE_ACQ_ONLY
    //    is used to generate the acquisition trigger
    // CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT
    //    is used to generate the acquisition trigger and trigger
    // output
    error_wrap("CAEN_DGTZ_SetSWTriggerMode Failed. ",
                         CAEN_DGTZ_SetSWTriggerMode, handle,
                         res->GlobalConfig.SWTriggerMode);

    error_wrap("CAEN_DGTZ_SetExtTriggerInputMode Failed. ",
                         CAEN_DGTZ_SetExtTriggerInputMode, handle,
                         res->GlobalConfig.EXTTriggerMode);

    // Acquisition mode comes in four flavors:
    // CAEN_DGTZ_SW_CONTROLLED
    //    Start/stop of the run takes place on software
    //    command (by calling CAEN_DGTZ_SWStartAcquisition )
    // CAEN_DGTZ_FIRST_TRG_CONTROLLED
    //    Run starts on the first trigger pulse (rising edge on
    // TRG-IN)    actual triggers start from the second pulse from there
    // CAEN_DGTZ_S_IN_CONTROLLED
    //    S-IN/GPI controller (depends on the board). Acquisition
    //    starts on edge of the GPI/S-IN
    // CAEN_DGTZ_LVDS_CONTROLLED
    //    VME ONLY, like S_IN_CONTROLLER but uses LVDS.
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

        // First, we make the channel mask
        uint32_t channel_mask = 0;
        for (auto ch_config : gr_configs) {
            channel_mask |= 1 << ch_config.Number;
        }

        uint32_t trg_mask = 0;
        for (auto ch_config : gr_configs) {
            trg_mask |= (ch_config.TriggerMask > 0) << ch_config.Number;
        }

        // Then enable those channels
        error_wrap("CAEN_DGTZ_SetChannelEnableMask Failed. ",
                             CAEN_DGTZ_SetChannelEnableMask,
                             handle, channel_mask);

        // Then enable if they are part of the trigger
        error_wrap("CAEN_DGTZ_SetChannelSelfTrigger Failed. ",
                             CAEN_DGTZ_SetChannelSelfTrigger, handle,
                             res->GlobalConfig.CHTriggerMode, trg_mask);

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
                                 CAEN_DGTZ_SetGroupDCOffset,
                                 handle, gr_config.Number,
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
        if (g_config.EXTAsGate) {
            write_bits(res, 0x810C, 1, 27);  // TRG-IN AND internal trigger,
            // and to serve as gate
            write_bits(res, 0x811C, 1, 10);  // TRG-IN as gate
        }

        if (trg_out) {
            write_bits(res, 0x811C, 0, 15);  // TRG-OUT based on internal signal
            write_bits(res, 0x811C, 0b00, 16, 2);  // TRG-OUT based
            // on internal signal
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
        res->LatestError = CAENError {
                "There was en error during setup! " + err_msg,  // ErrorMessage
                static_cast<CAEN_DGTZ_ErrorCode>(latest_err),  // ErrorCode
                true};  // isError
    }
}

template<typename T>
void CAEN<T>::EnableAcquisition() noexcept {
    if (_has_error || !_is_connected) {
        return;
    }

    int& handle = _caen_api_handle;

    // We need this to interface to the better C++ API
    char* tmp_ptr = nullptr;
    int err = CAEN_DGTZ_MallocReadoutBuffer(handle,
        &tmp_ptr, &res->Data.TotalSizeBuffer);

    // Now we create our own with better memory management
    res->Data.Buffer.reset(new char[res->Data.TotalSizeBuffer]);

    // We copy the values because we do not know if they have significance
    // for the CAEN API
    std::memcpy(res->Data.Buffer.get(), tmp_ptr,
        sizeof(char)*res->Data.TotalSizeBuffer);

    err |= CAEN_DGTZ_ClearData(handle);
    err |= CAEN_DGTZ_SWStartAcquisition(handle);

    // We delete the c pointer as all the memory should be in our C++ pointer
    delete tmp_ptr;

    if (err < 0) {
        res->LatestError = CAENError {
            "There was en error while trying to enable"
            "acquisition or allocate memory for buffers.",  // ErrorMessage
            static_cast<CAEN_DGTZ_ErrorCode>(err),  // ErrorCode
            true  // isError
        };
    }
}

template<typename T>
void CAEN<T>::DisableAcquisition() noexcept {
    if (_has_error || !_is_connected) {
        return;
    }

    auto err = CAEN_DGTZ_SWStopAcquisition(_caen_api_handle);

    if (err < 0) {
        res->LatestError = CAENError {
            "There was en error while trying to disable"
            "acquisition.",  // ErrorMessage
            err,  // ErrorCode
            true  // isError
        };
    }
}

template<typename T>
void CAEN<T>::WriteRegister(uint32_t&& addr, const uint32_t& value) noexcept {
    if (_has_error || !_is_connected) {
        return;
    }

    auto err = CAEN_DGTZ_WriteRegister(_caen_api_handle, addr, value);
    if (err < 0) {
        res->LatestError = CAENError {
            "There was en error while trying to write"
            "to register.",  // ErrorMessage
            err,  // ErrorCode
            true  // isError
        };
    }
}

template<typename T>
void CAEN<T>::ReadRegister(uint32_t&& addr, uint32_t& value) noexcept {
    if (_has_error || !_is_connected) {
        return;
    }

    auto err = CAEN_DGTZ_ReadRegister(_caen_api_handle, addr, &value);
    if (err < 0) {
        res->LatestError = CAENError {
            "There was en error while trying to read"
            "to register " + std::to_string(addr),  // ErrorMessage
            err,  // ErrorCode
            true  // isError
        };
    }
}

template<typename T>
void CAEN<T>::WriteBits(uint32_t&& addr,
    const uint32_t& value, uint8_t pos, uint8_t len) noexcept {
    if (_has_error || !_is_connected) {
        return;
    }

    // First read the register
    uint32_t read_word = 0;
    auto err = CAEN_DGTZ_ReadRegister(_caen_api_handle, addr, &read_word);

    if (err < 0) {
        res->LatestError = CAENError {
            "There was en error while trying to read"
            "to register " + std::to_string(addr),  // ErrorMessage
            err,  // ErrorCode
            true  // isError
        };
    }

    uint32_t bit_mask = ~(((1 << len) - 1) << pos);
    read_word = read_word & bit_mask; //mask the register value

    // Get the lowest bits of value and shifted to the correct position
    uint32_t value_bits = (value & ((1 << len) - 1)) << pos;
    // Combine masked value read from register with new bits
    err = CAEN_DGTZ_WriteRegister(_caen_api_handle, addr, read_word | value_bits);

    if (err < 0) {
        res->LatestError = CAENError {
            "There was en error while trying to write"
            "to register " + std::to_string(addr),  // ErrorMessage
            err,  // ErrorCode
            true  // isError
        };
    }
}

template<typename T>
void CAEN<T>::SoftwareTrigger() noexcept {
    if (_has_error || !_is_connected) {
        return;
    }

    if (res->LatestError.isError) {
        return;
    }

    auto err = CAEN_DGTZ_SendSWtrigger(_caen_api_handle);

    if (err < 0) {
        res->LatestError = CAENError {
            "Failed to send a software trigger",  // ErrorMessage
            err,  // ErrorCode
            true  // isError
        };
    }
}

template<typename T>
uint32_t CAEN<T>::GetEventsInBuffer() noexcept {
    uint32_t events = 0;
    // For 5730 it is the register 0x812C
    read_register(res, 0x812C, events);

    return events;
}

template<typename T>
void CAEN<T>::RetrieveData() noexcept {
    int& handle = _caen_api_handle;

    if (_has_error || !_is_connected) {
        return;
    }

    int err = CAEN_DGTZ_ReadData(handle,
        CAEN_DGTZ_ReadMode_t::CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,
        res->Data.Buffer.get(),
        &res->Data.DataSize);

    if (err >= 0) {
        err = CAEN_DGTZ_GetNumEvents(handle,
            res->Data.Buffer.get(),
            res->Data.DataSize,
            &res->Data.NumEvents);
    }

    if (err < 0) {
        res->LatestError = CAENError {
            "Failed to retrieve data!",  // ErrorMessage
            static_cast<CAEN_DGTZ_ErrorCode>(err),  // ErrorCode
            true  // isError
        };
    }
}

template<typename T>
bool CAEN<T>::RetrieveDataUntilNEvents(uint32_t& n, bool debug_info) noexcept {
    int& handle = _caen_api_handle;

    if (_has_error->start_rate_calculation) {
        res->ts = std::chrono::high_resolution_clock::now();
        res->start_rate_calculation = true;
        // When starting statistics mode, mark time
    }

    if (_has_error || !_is_connected) {
        return false;
    }

    if (n >= res->CurrentMaxBuffers) {
        if (get_events_in_buffer(res) < res->CurrentMaxBuffers) {
            return false;
        }
    } else if (get_events_in_buffer(res) < n) {
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
        res->Data.Buffer.get(),
        &res->Data.DataSize);

    if (res->LatestError.ErrorCode < 0){
        res->LatestError.isError = true;
        res->LatestError.ErrorMessage = "There was an error when trying "
        "to read buffer";
        return false;
    }

    res->LatestError.ErrorCode = CAEN_DGTZ_GetNumEvents(handle,
            res->Data.Buffer.get(),
            res->Data.DataSize,
            &res->Data.NumEvents);

    // Calculate trigger rate
    res->te = std::chrono::high_resolution_clock::now();
    res->t_us = std::chrono::
        duration_cast<std::chrono::microseconds>(res->te - res->ts).count();
    res->n_events = res->Data.NumEvents;
    res->ts = res->te;
    res->duration += res->t_us;
    res->trg_count += res->n_events;

    if (debug_info){
        spdlog::info(fmt::format("#Triggers: {:7d}, Duration: {:-6.2f}s, Inst Rate:{:-8.2f}Hz, Accl Rate:{:-8.2f}Hz",
            res->trg_count,
            res->duration/1e6,
            res->n_events*1e6/res->t_us,
            res->trg_count*1e6/res->duration));
    }

    if (res->LatestError.ErrorCode < 0){
        res->LatestError.isError = true;
        res->LatestError.ErrorMessage = "There was an error when trying"
        "to recover number of events from buffer";
        return false;
    }

    return true;
}

template<typename T>
void CAEN::ExtractEvent(const uint32_t& i, CAENEvent& evt) noexcept {
    int& handle = _caen_api_handle;

    if (_has_error || !_is_connected ) {
        return;
    }

    if (!evt) {
        evt = std::make_shared<caenEvent>(handle);
    }

        CAEN_DGTZ_GetEventInfo(handle,
            res->Data.Buffer.get(),
            res->Data.DataSize,
            i,
            &evt->Info,
            &evt->DataPtr);

        CAEN_DGTZ_DecodeEvent(handle,
            evt->DataPtr,
            reinterpret_cast<void**>(&evt->Data));
}

template<typename T>
void CAEN::ClearData() noexcept {
    if (_has_error || !_is_connected) {
        return;
    }

    int& handle = _caen_api_handle;
    int err = CAEN_DGTZ_SWStopAcquisition(handle);
    err |= CAEN_DGTZ_ClearData(handle);
    err |= CAEN_DGTZ_SWStartAcquisition(handle);
    if (err < 0) {
        res->LatestError = CAENError {
            "Failed to send a software trigger",  // ErrorMessage
            static_cast<CAEN_DGTZ_ErrorCode>(err),  // ErrorCode
            true  // isError
        };
    }
}

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
std::string sbc_save_func(CAENEvent&, CAEN&) noexcept;

/// End File functions

}   // namespace SBCQueens
#endif
