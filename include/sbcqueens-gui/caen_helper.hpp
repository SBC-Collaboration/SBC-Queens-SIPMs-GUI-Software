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
#include "logger_helpers.hpp"

namespace SBCQueens {

enum class CAENDigitizerFamilies {
#ifndef NDEBUG
    DEBUG = 0,
#endif
    x725, x730, x740, x742, x743, x751
};

// Translates the CAEN API error code to a string.
constexpr std::string translate_caen_error_code(const CAEN_DGTZ_ErrorCode& err);

// This list is incomplete
enum class CAENConnectionType {
    USB,
    A4818
};

// This list is incomplete
enum class CAENDigitizerModel {
#ifndef NDEBUG
    DEBUG = 0,
#endif
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

    // Max number of internal buffers the digitizer can have.
    uint32_t MaxNumBuffers = 1024;

    // Constant to transform Nloc to Recordlength
    float NLOCToRecordLength = 1.0f;

    // Voltage ranges the digitizer has.
    std::vector<double> VoltageRanges = {};
};

// This is here so we can transform string to enums
// If the key does not exist, this will throw an error
const static inline std::unordered_map<std::string, CAENDigitizerModel>
    CAENDigitizerModelsMap {
#ifndef NDEBUG
        {"DEBUG", CAENDigitizerModel::DEBUG},
#endif
        {"DT5730B", CAENDigitizerModel::DT5730B},
        {"DT5740D", CAENDigitizerModel::DT5740D},
        {"V1740D", CAENDigitizerModel::V1740D}
};

// These links all the enums with their constants or properties that
// are fixed per digitizer
const static inline std::unordered_map<CAENDigitizerModel, CAENDigitizerModelConstants>
    CAENDigitizerModelsConstantsMap {
        {CAENDigitizerModel::DEBUG, CAENDigitizerModelConstants {
            8,  // ADCResolution
            100e3,  //  AcquisitionRate
            1024ul,  // MemoryPerChannel
            1,  // NumChannels
            0,  // NumberOfGroups, 0 -> no groups
            1,  // NumChannelsPerGroup
            1024,  // MaxNumBuffers
            10.0f,  // NLOCToRecordLength
            {1.0}  // VoltageRanges
        }},
        {CAENDigitizerModel::DT5730B, CAENDigitizerModelConstants {
            14,  // ADCResolution
            500e6,  //  AcquisitionRate
            static_cast<uint32_t>(5.12e6),  // MemoryPerChannel
            8,  // NumChannels
            0,  // NumberOfGroups, 0 -> no groups
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
            4,  // NumChannelsPerGroup
            8,  // NumberOfGroups
            1024,  // MaxNumBuffers
            1.5f,  // NLOCToRecordLength
            {2.0, 10.0}  // VoltageRanges
        }},
        {CAENDigitizerModel::V1740D, CAENDigitizerModelConstants {
            12,  // ADCResolution
            62.5e6,  //  AcquisitionRate
            static_cast<uint32_t>(192e3),  // MemoryPerChannel
            64,  // NumChannels
            8,  // NumChannelsPerGroup
            8,  // NumberOfGroups
            1024,  // MaxNumBuffers
            1.5f,  // NLOCToRecordLength
            {2.0}  // VoltageRanges
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
    uint32_t RecordLength = 100;

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

    // This feature is for x730, x740
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
    // Channel or group number
    bool Enabled = 0;

    // Mask of channels within the group enabled to trigger
    // If Channel, if its != 0 then its enabled
    ChannelsMask TriggerMask;

    // Mask of enabled channels within the group
    // Ignored for single channels.
    ChannelsMask AcquisitionMask;

    // 0x8000 no offset if 16 bit DAC
    // 0x1000 no offset if 14 bit DAC
    // 0x400 no offset if 14 bit DAC
    // DC offsets of each channel in the group
    uint32_t DCOffset = 0x8000;
    std::array<uint8_t, 8> DCCorrections = {0, 0, 0, 0, 0, 0, 0, 0};

    // For DT5730
    // 0 = 2Vpp, 1 = 0.5Vpp
    // DC range of the group or channel
    uint8_t DCRange = 0;

    // In ADC counts
    uint32_t TriggerThreshold = 0;
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

        CAENData() : Buffer{nullptr, &CAENData::_clear_memory} { }
    };

    static inline std::unordered_map<int, bool> _connection_info_map;

    // THis is a number that the CAEN API uses to manage its own resources.
    int _caen_api_handle = -1;
    bool _is_connected = false;
    bool _is_acquiring = false;
    CAEN_DGTZ_ErrorCode _err_code;

    bool _has_error = false;
    bool _has_warning = false;

    // This holds the latest raw CAEN data
    CAENData _caen_raw_data;

    // Communicated with the outside world: errors, warnings and debug msgs
    // Assumes it is a pointer of any form and this class does not manage
    // its deletion
    Logger _logger;

    CAENGlobalConfig _global_config;
    std::array<CAENGroupConfig, 8> _group_configs;
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
    void _print_if_err(const std::string& CAEN_func_name,
        const std::string& location,
        const std::string& extra_msg = "") noexcept {

        const std::string expression_str =
            "{1} at {0} in CAEN function named {2} with CAEN message {3}. "
            "Additional info: {2}";

        switch (_err_code) {
        case CAEN_DGTZ_ErrorCode::CAEN_DGTZ_Success:
            break;

        case CAEN_DGTZ_ErrorCode::CAEN_DGTZ_ChannelBusy:
        case CAEN_DGTZ_ErrorCode::CAEN_DGTZ_FunctionNotAllowed:
        case CAEN_DGTZ_ErrorCode::CAEN_DGTZ_Timeout:
        case CAEN_DGTZ_ErrorCode::CAEN_DGTZ_DigitizerAlreadyOpen:
        case CAEN_DGTZ_ErrorCode::CAEN_DGTZ_DPPFirmwareNotSupported:
        case CAEN_DGTZ_ErrorCode::CAEN_DGTZ_NotYetImplemented:
            _has_warning = true;
            _logger->warning(expression_str, "Warning", location,
                CAEN_func_name, translate_caen_error_code(_err_code),
                extra_msg);
            break;
        default:
            _has_error = true;
            _logger->warning(expression_str, "Warning", location,
                CAEN_func_name, translate_caen_error_code(_err_code),
                extra_msg);
        }
    }

    CAENDigitizerFamilies __get_family(const CAENDigitizerModel& model) {
        switch(model) {
        case CAENDigitizerModel::DT5740D:
        case CAENDigitizerModel::V1740D: 
            return CAENDigitizerFamilies::x740;
        default:
        case CAENDigitizerModel::DT5730B: 
            return CAENDigitizerFamilies::x730;
        }
    }

 public:
    // Family 
    const CAENDigitizerFamilies Family;
    // Model
    const CAENDigitizerModel Model;
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
        _logger{logger},
        Family{__get_family(model)}
        Model{model},
        ModelConstants{CAENDigitizerModelsConstantsMap.at(model)},
        ConnectionType{ct},
        LinkNum{ln},
        ConetNode{cn},
        VMEBaseAddress{addr}
    {
        if (not logger) {
            throw "Logger is not an existing resource.";
        }

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
            _print_if_err("CAEN_DGTZ_OpenDigitizer", __FUNCTION__, "Failed"
                "to open the port to the digitizer.");
        } else {
            _logger->info("Connected resource with handle {0} with "
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
            _print_if_err("CAEN_DGTZ_SWStopAcquisition", __FUNCTION__,
                "Failed to stop acquisition during cleaning of "
                "resources, what went wrong?");

            _err_code = CAEN_DGTZ_CloseDigitizer(_caen_api_handle);
            _print_if_err("CAEN_DGTZ_CloseDigitizer", __FUNCTION__,
                "Failed to close digitizer, whaaat?");
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

    constexpr uint32_t GetCommTransferRate() {
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

    // Turns a time (in nanoseconds) into counts
    // Reminder that some digitizers take data in chunks or only allow
    // some record lengths, so some record lengths are not possible.
    // Ex: x5730 record length can only be multiples of 10
    constexpr uint32_t TimeToRecordLength(const double& nsTime) noexcept {
        return static_cast<uint32_t>(nsTime*1e-9*GetCommTransferRate());
    }

    // Turns a voltage threshold counts to ADC cts
    constexpr uint32_t VoltThresholdCtsToADCCts(const uint32_t& cts) noexcept {
        const uint32_t kVthresholdRangeCts = std::exp2(16);
        uint32_t ADCRange = std::exp2(ModelConstants.ADCResolution);

        uint32_t out = ADCRange*cts;
        return out / kVthresholdRangeCts;
    }

    // Turns a voltage (V) into trigger counts
    constexpr uint32_t VoltToThresholdCounts(const double& volt) noexcept {
        const uint32_t kVthresholdRangeCts = std::exp2(16);
        return static_cast<uint32_t>(volt / kVthresholdRangeCts);
    }

    // Turns a voltage (V) into count offset
    constexpr uint32_t VoltOffsetToCountOffset(const double& volt) noexcept {
        uint32_t ADCRange = std::exp2(res->ModelConstants.ADCResolution);
        return static_cast<uint32_t>(volt / ADCRange);
    }

    // Calculates the max number of buffers for a given record length
    constexpr uint32_t CalculateMaxBuffers() noexcept {
        // Instead of using the stored RecordLength
        // We will let CAEN functions do all the hard work for us
        // uint32_t rl = res->GlobalConfig.RecordLength;
        uint32_t rl = 0;

        // Cannot be higher than the memory per channel
        auto mem_per_ch = ModelConstants.MemoryPerChannel;
        uint32_t nloc = 0;

        // This contains nloc which if multiplied by the correct
        // constant, will give us the record length exactly
        ReadRegister(res, 0x8020, nloc);
        rl =  ModelConstants.NLOCToRecordLength*nloc;

        // Then we calculate the actual num buffs but...
        auto max_num_buffs = mem_per_ch / rl;

        // It cannot be higher than the max buffers
        max_num_buffs = max_num_buffs >= res->ModelConstants.MaxNumBuffers ?
            res->ModelConstants.MaxNumBuffers : max_num_buffs;

        // It can only be a power of 2
        uint32_t real_max_buffs
            = static_cast<uint32_t>(exp2(floor(log2(max_num_buffs))));

        // Unless this is enabled, then it is always that number minus one.
        // Except when the buffers is 1
        if (GlobalConfig.MemoryFullModeSelection) {
            if (real_max_buffs > 2) {
                real_max_buffs--;
            } else {
                real_max_buffs = 1;
            }
        }

        // Phew, we are done!
        return real_max_buffs;
    }
};


/// General CAEN functions

template<typename T>
void CAEN<T>::Reset() noexcept {
    if (_has_error) {
        return;
    }

    _err_code = CAEN_DGTZ_Reset(_caen_api_handle);
    _print_if_err("CAEN_DGTZ_Reset", __FUNCTION__);
}

template<typename T>
void CAEN<T>::Setup(const CAENGlobalConfig& g_config,
    const std::vector<CAENGroupConfig>& gr_configs) noexcept {
    if (_has_error || !_is_connected) {
        return;
    }

    Reset();
    DisableAcquisition():

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
    // No need to delete this pointer as its ownership is taken by the
    // smart pointer later
    char* tmp_ptr = nullptr;
    _err_code = CAEN_DGTZ_MallocReadoutBuffer(handle, &tmp_ptr,
        &Data.TotalSizeBuffer);
    _print_if_err("CAEN_DGTZ_MallocReadoutBuffer", __FUNCTION__);

    // Now we create our own with better memory management
    Data.Buffer.reset(tmp_ptr);

    _err_code = CAEN_DGTZ_ClearData(handle);
    _print_if_err("CAEN_DGTZ_ClearData", __FUNCTION__);

    _err_code = CAEN_DGTZ_SWStartAcquisition(handle);
    _print_if_err("CAEN_DGTZ_SWStartAcquisition", __FUNCTION__);

    if (not _has_error) {
        _is_acquiring = true;
    }
}

template<typename T>
void CAEN<T>::DisableAcquisition() noexcept {
    if (_has_error || !_is_connected || !_is_acquiring) {
        return;
    }

    _err_code = = CAEN_DGTZ_SWStopAcquisition(_caen_api_handle);
    _print_if_err("CAEN_DGTZ_SWStopAcquisition", __FUNCTION__);
    _is_acquiring = false;
}

template<typename T>
void CAEN<T>::WriteRegister(uint32_t&& addr, const uint32_t& value) noexcept {
    if (_has_error || !_is_connected) {
        return;
    }

    _err_code = = CAEN_DGTZ_WriteRegister(_caen_api_handle, addr, value);
    _print_if_err("CAEN_DGTZ_WriteRegister", __FUNCTION__);
}

template<typename T>
void CAEN<T>::ReadRegister(uint32_t&& addr, uint32_t& value) noexcept {
    if (_has_error || !_is_connected) {
        return;
    }

    _err_code = = CAEN_DGTZ_ReadRegister(_caen_api_handle, addr, &value);
    _print_if_err("CAEN_DGTZ_ReadRegister", __FUNCTION__);
}

template<typename T>
void CAEN<T>::WriteBits(uint32_t&& addr,
    const uint32_t& value, uint8_t pos, uint8_t len) noexcept {
    if (_has_error || !_is_connected) {
        return;
    }

    // First read the register
    uint32_t read_word = 0;
    _err_code = CAEN_DGTZ_ReadRegister(_caen_api_handle, addr, &read_word);
    _print_if_err("CAEN_DGTZ_ReadRegister", __FUNCTION__);

    uint32_t bit_mask = ~(((1 << len) - 1) << pos);
    read_word = read_word & bit_mask; //mask the register value

    // Get the lowest bits of value and shifted to the correct position
    uint32_t value_bits = (value & ((1 << len) - 1)) << pos;
    // Combine masked value read from register with new bits
    _err_code = CAEN_DGTZ_WriteRegister(_caen_api_handle, addr, read_word | value_bits);
    _print_if_err("CAEN_DGTZ_WriteRegister", __FUNCTION__);
}

template<typename T>
void CAEN<T>::SoftwareTrigger() noexcept {
    if (_has_error || !_is_connected) {
        return;
    }

    _err_code = CAEN_DGTZ_SendSWtrigger(_caen_api_handle);
    _print_if_err("CAEN_DGTZ_SendSWtrigger", __FUNCTION__);
}

template<typename T>
uint32_t CAEN<T>::GetEventsInBuffer() noexcept {
    uint32_t events = 0;
    // For 5730 it is the register 0x812C
    ReadRegister(res, 0x812C, events);

    return events;
}

template<typename T>
void CAEN<T>::RetrieveData() noexcept {
    int& handle = _caen_api_handle;

    if (_has_error || !_is_connected || !_is_acquiring) {
        return;
    }

    _err_code = CAEN_DGTZ_ReadData(handle,
        CAEN_DGTZ_ReadMode_t::CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,
        res->Data.Buffer.get(),
        &res->Data.DataSize);
    _print_if_err("CAEN_DGTZ_ReadData", __FUNCTION__);

    _err_code = CAEN_DGTZ_GetNumEvents(handle,
        res->Data.Buffer.get(),
        res->Data.DataSize,
        &res->Data.NumEvents);
    _print_if_err("CAEN_DGTZ_GetNumEvents", __FUNCTION__);
}

template<typename T>
bool CAEN<T>::RetrieveDataUntilNEvents(uint32_t& n, bool debug_info) noexcept {
    int& handle = _caen_api_handle;

    if (_has_error || !_is_connected || !_is_acquiring) {
        return false;
    }

    if (n >= res->CurrentMaxBuffers) {
        if (GetEventsInBuffer() < res->CurrentMaxBuffers) {
            return false;
        }
    } else if (GetEventsInBuffer() < n) {
        return false;
    }

    // There is a weird quirk related to this function that BLOCKS
    // the software essentially killing it.
    // If TriggerOverlapping is allowed sometimes this function
    // returns more data than Buffer it can hold, why? Idk
    // but so far with the software as is, it won't work with that
    // so dont do it!
    _err_code = CAEN_DGTZ_ReadData(handle,
        CAEN_DGTZ_ReadMode_t::CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,
        res->Data.Buffer.get(),
        &res->Data.DataSize);
    _print_if_err("CAEN_DGTZ_ReadData", __FUNCTION__);

    _err_code = CAEN_DGTZ_GetNumEvents(handle,
            res->Data.Buffer.get(),
            res->Data.DataSize,
            &res->Data.NumEvents);
    _print_if_err("CAEN_DGTZ_GetNumEvents", __FUNCTION__);

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
    _err_code = CAEN_DGTZ_SWStopAcquisition(handle);
    _err_code = CAEN_DGTZ_ClearData(handle);
    _err_code = CAEN_DGTZ_SWStartAcquisition(handle);

}

/// End Data Acquisition functions
//
/// Mathematical functions



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
