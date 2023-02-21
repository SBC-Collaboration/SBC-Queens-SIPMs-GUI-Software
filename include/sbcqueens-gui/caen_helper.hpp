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
#include <stdexcept>

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

// Translates the CAEN API error code to a string.
constexpr std::string translate_caen_error_code(const CAEN_DGTZ_ErrorCode&);

// This list is incomplete
enum class CAENConnectionType {
    USB,
    A4818,
    OpticalLink, // Not supported
    Ethernet_V4718, // Only available for V4718 (not supported)
    USB_V4718 // not supported
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

// Events structure: holds the raw data of the event, the info (timestamp),
// and the pointer to the point in the original buffer.
// This uses CAEN functions to allocate memory, so if handle does not
// have an associated digitizer to it, it will not work.
// Create events only after a setup(...) call has been made for best
// results.
class CAENEvent {
    int _handle = 0;
    // Possibly the most memory unsafe part of all CAEN software.
    // Remember kids, do not write functions that take void**
    // that modify their internal values

    // A pointer to the location in the CAENData (see below)
    // DOES NOT MANAGE DATA
    char* DataPtr = nullptr;
    // The reinterpreted data as uint16_ts with sizes.
    // Must be pointer because that is what the CAEN API expects.
    CAEN_DGTZ_UINT16_EVENT_t* Data = nullptr;
    // Contains the Info.
    CAEN_DGTZ_EventInfo_t Info = CAEN_DGTZ_EventInfo_t{};

    CAEN_DGTZ_ErrorCode _err_code = CAEN_DGTZ_ErrorCode::CAEN_DGTZ_Success;

    // Why is this here? Glad you asked! See the destructor
    std::shared_ptr<int> _shared_ptr_trick;

 public:
    // Default constructor does not allocate memory.
    CAENEvent() = default;
    // Creating an Event using a handle allocates memory
    explicit CAENEvent(const int& handle)
        : _handle(handle), _shared_ptr_trick(new int) {
        // This will only allocate memory if handle does
        // has an associated digitizer to it.
        _err_code = CAEN_DGTZ_AllocateEvent(_handle,
                                            reinterpret_cast<void**>(&Data));
    }

    // If handle is released before this event is freed,
    // it will cause a memory leak, maybe?
    ~CAENEvent() {
        /*
            Here is where _shared_ptr_trick becomes useful.
            By default, the copy operator of this class will be enabled because
            the copy of the raw pointers and normal types will be copied
            directly. The shared_ptr will also be transferred. This allows us
            to a neat trick.
            Let's say we have:

            CAENEvent evt1, evt2;
            evt1 = CAENEvent(42); // now evt1 has allocated value and
                                  // _shared_ptr_trick does not hold a nullptr
            evt2 = evt1           // evt2 now copies Data, Info and created
                                  // a new _shared_ptr_trick which shares
                                  // an int
            ...
            destroy(evt1);        // now evt1 goes out of scope but because
                                  // _shared_ptr_trick is not unique
                                  // nothing got freed!
            destroy(evt2);        // now everything got cleared!

            Now CAENEvent acts like a shared_ptr!
        */
        if (not _shared_ptr_trick or Data == nullptr) {
            return;
        }
        if (_shared_ptr_trick.unique()) {
            _err_code = CAEN_DGTZ_FreeEvent(_handle,
                                            reinterpret_cast<void**>(&Data));
            if(_err_code < 0) {
                std::cout << "Fatal error at ~CAENEvent()"
                        "Failed to free memory with error: " <<
                        translate_caen_error_code(_err_code) << "\n";
                throw std::runtime_error("Fatal error at ~CAENEvent with "
                                         "CAEN Error :" +
                                         translate_caen_error_code(_err_code));
            }
        }
    }

    const CAEN_DGTZ_ErrorCode& getError() {
        return _err_code;
    }

    // A read-only access to the event data.
    [[nodiscard]] const CAEN_DGTZ_UINT16_EVENT_t* getData() const {
        return Data;
    }

    // A read-only access to the event info.
    const CAEN_DGTZ_EventInfo_t& getInfo() {
        return Info;
    }

    // This is a stupid function, char* is mutable so any interfacing
    // code will be unsafe by default. Blame CAEN, not me. Take max precautions.
    //
    // This grabs the data from data_ptr, CAEN magic happens and returns
    // the info of the event, and a pointer to its location in data_ptr
    CAEN_DGTZ_ErrorCode getEventInfo(char* data_ptr,
                                     const uint32_t& data_size,
                                     const int32_t& i) noexcept {
        _err_code = CAEN_DGTZ_GetEventInfo(_handle,
                                           data_ptr,
                                           data_size,
                                           i,
                                           &Info,
                                           &DataPtr);
        return _err_code;
    }

    // Decodes the information at the pointer found in getEventInfo
    //
    // Cannot decode without calling getEventInfo at least once. If
    // someone the data used in data_ptr is destroyed this will return an error
    CAEN_DGTZ_ErrorCode decodeEvent() {
        _err_code = CAEN_DGTZ_DecodeEvent(_handle,
                              DataPtr,
                              reinterpret_cast<void**>(&Data));
        return _err_code;
    }
};

template<typename Logger = std::shared_ptr<iostream_wrapper>,
         size_t EventBufferSize = 1024>
class CAEN {
    // Holds the CAEN raw data, the size of the buffer, the size
    // of the data and number of events
    // pointer Buffer is meant to be allocated using CAEN functions
    // after a setup(...) call.
    class CAENData {
        Logger _logger;
        CAEN_DGTZ_ErrorCode _err_code = CAEN_DGTZ_ErrorCode::CAEN_DGTZ_Success;

        char* _caen_malloc(const int& handle) noexcept {
            char* tmp_ptr;
            _err_code = CAEN_DGTZ_MallocReadoutBuffer(handle, &tmp_ptr,
                                                      &TotalSizeBuffer);
        }
     public:
        // Unsafe C buffer. Only this class has access to CAENData
        // Therefore all the damage is limited to us not the user.
        char* Buffer;
        uint32_t TotalSizeBuffer = 0;
        uint32_t DataSize = 0;
        uint32_t NumEvents = 0;

        CAENData(Logger logger, const int& handle) :
            _logger{logger},
            Buffer{_caen_malloc(handle)} { }

        const CAEN_DGTZ_ErrorCode& getError() {
            return _err_code;
        }

        ~CAENData() {
            _err_code = CAEN_DGTZ_FreeReadoutBuffer(&Buffer);

            if(_err_code < 0) {
                _logger.error("Fatal error at CAENData -> _clear_data_mem."
                    "Failed to free memory with error: {}",
                    translate_caen_error_code(_err_code));
                throw;
            }
        }
    };

    static inline std::unordered_map<int, bool> _connection_info_map;

    // THis is a number that the CAEN API uses to manage its own resources.
    int _caen_api_handle = -1;
    bool _is_connected = false;
    bool _is_acquiring = false;

    // CAEN ENUMS. Holds the latest error thrown by any of the CAEN APIs funcs
    CAEN_DGTZ_ErrorCode _err_code = CAEN_DGTZ_ErrorCode::CAEN_DGTZ_Success;

    bool _has_error = false;
    bool _has_warning = false;

    // Communicated with the outside world: errors, warnings and debug msgs
    // Assumes it is a pointer of any form and this class does not manage
    // its deletion
    Logger _logger;

    // This holds the latest raw CAEN data
    CAENData _caen_raw_data;
    // Contains information about the configuration of the digitizer
    CAENGlobalConfig _global_config;
    // Contains information about all of the groups, see CAENGroupConfig
    // for more information. Currently it is only possible to have up to
    // 8 groups from any digitizer.
    std::array<CAENGroupConfig, 8> _group_configs;

    CAEN_DGTZ_BoardInfo_t _board_info;
    uint32_t _current_max_buffers = 0;

    std::array<CAENEvent, EventBufferSize> _events;

    // Translates the connection info data to a single number that should
    // be unique.
    constexpr uint64_t _hash_connection_info(const CAENConnectionType& ct,
        const int& ln, const int& cn, const uint32_t& addr) noexcept {
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
    void _print_if_err(std::string_view CAEN_func_name,
                       std::string_view location,
                       std::string_view extra_msg = "") noexcept {

        const std::string expression_str =
            "{} at {} in CAEN function named {} with CAEN message {}. "
            "Additional info: {}";

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

    CAENDigitizerFamilies _get_family(const CAENDigitizerModel& model) {
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
        _caen_raw_data{logger},
        Family{_get_family(model)},
        Model{model},
        ModelConstants{CAENDigitizerModelsConstantsMap.at(model)},
        ConnectionType{ct},
        LinkNum{ln},
        ConetNode{cn},
        VMEBaseAddress{addr}
    {
        if (not logger) {
            throw std::invalid_argument("Logger is not an existing resource.");
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

        _is_connected = (_err_code < 0) and (_caen_api_handle > 0);
        if (not _is_connected) {
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

    // Public members
    // Check whenever the port is connected
    bool IsConnected() noexcept { return _is_connected; }

    bool HasError() noexcept { return _has_error; }

    void ResetWarning() noexcept { _has_warning = false; }
    bool HasWarning() noexcept { return _has_warning; }

    const auto& GetBoardInfo() { return _board_info; }
    const auto& GetGlobalConfiguration() { return _global_config; }
    const auto& GetGroupConfigurations() { return _group_configs; }
    const auto& GetNumberOfEvents() { return _caen_raw_data.NumEvents; }

    void Setup(const CAENGlobalConfig&,
        const std::array<CAENGroupConfig, 8>&) noexcept;
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
    void WriteRegister(uint32_t&& addr, const uint32_t& value) noexcept;
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
    // Decodes event i from the data retrieved by any call from RetrieveData
    // If i is out of bounds, returns the event at the end of the buffer.
    // if there is an error during acquisition, this returns a nullptr;
    const CAENEvent* DecodeEvent(const uint32_t& i) noexcept;
    // Decodes the latest acquired events.
    void DecodeEvents() noexcept;
    // Clears the digitizer buffer. It stops the acquisition and resumes it
    // after clearing the data.
    void ClearData() noexcept;
    // Returns a const pointer to the event held @ index i.
    // If i value is higher the latest acquired number of events, it returns
    // the last event. Use GetNumberOfevents() to check for the number
    // of events in memory
    const CAENEvent* GetEvent(const std::size_t& i) noexcept {
        if (i > _caen_raw_data.NumEvents) {
            return &_events[_caen_raw_data.NumEvents - 1];
        }

        return &_events[i];
    }

    bool start_rate_calculation = false;
    std::chrono::high_resolution_clock::time_point ts, te;
    uint64_t t_us, n_events;
    uint64_t trg_count = 0, duration = 0;

    const uint32_t& GetCurrentPossibleMaxBuffer() noexcept {
        return _current_max_buffers;
    }

    uint32_t GetCommTransferRate() noexcept {
        if(ConnectionType == CAENConnectionType::USB){
            return 15000000u;  // S/s
        } else if (ConnectionType == CAENConnectionType::A4818) {
            return 40000000u;
        }

        return 0u;
    }

    // Returns the channel voltage range. If channel does not exist
    // returns 0
    double GetVoltageRange(const uint8_t& gr_n) noexcept {
        try {
            auto config = _group_configs[gr_n];
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
    uint32_t TimeToRecordLength(const double& nsTime) noexcept {
        return static_cast<uint32_t>(nsTime*1e-9*GetCommTransferRate());
    }

    // Turns a voltage threshold counts to ADC cts
    uint32_t VoltThresholdCtsToADCCts(const uint32_t& cts) noexcept {
        const uint32_t kVthresholdRangeCts = std::exp2(16);
        const uint32_t ADCRange = std::exp2(ModelConstants.ADCResolution);
        return ADCRange*cts / kVthresholdRangeCts;
    }

    // Turns a voltage (V) into trigger counts
    uint32_t VoltToThresholdCounts(const double& volt) noexcept {
        const uint32_t kVthresholdRangeCts = std::exp2(16);
        return static_cast<uint32_t>(volt / kVthresholdRangeCts);
    }

    // Turns a voltage (V) into count offset
    uint32_t VoltOffsetToCountOffset(const double& volt) noexcept {
        uint32_t ADCRange = std::exp2(ModelConstants.ADCResolution);
        return static_cast<uint32_t>(volt / ADCRange);
    }
};

/// General CAEN functions
template<typename T, size_t N>
void CAEN<T, N>::Reset() noexcept {
    if (_has_error) {
        return;
    }

    _err_code = CAEN_DGTZ_Reset(_caen_api_handle);
    _print_if_err("CAEN_DGTZ_Reset", __FUNCTION__);
}

template<typename T, size_t N>
void CAEN<T, N>::Setup(const CAENGlobalConfig& global_config,
    const std::array<CAENGroupConfig, 8>& gr_configs) noexcept {
    if (_has_error or not _is_connected) {
        return;
    }
    // First, we disable acquisition just to make sure.
    // as some parameters can only be changed while the acquisition
    // is disabled.
    DisableAcquisition();
    // Second, we put the digitizer in a known state.
    Reset();

    int &handle = _caen_api_handle;

    // Now, global configuration
    // Global config
    _global_config = global_config;

    _err_code = CAEN_DGTZ_GetInfo(handle, &_board_info);
    _print_if_err("CAEN_DGTZ_GetInfo", __FUNCTION__);

    _err_code = CAEN_DGTZ_SetMaxNumEventsBLT(handle, _global_config.MaxEventsPerRead);
    _print_if_err("CAEN_DGTZ_SetMaxNumEventsBLT", __FUNCTION__);

    _err_code = CAEN_DGTZ_SetRecordLength(handle, _global_config.RecordLength);
    _print_if_err("CAEN_DGTZ_SetRecordLength", __FUNCTION__);

    // We need to ask the digitizer what is the actual record length is using
    // to keep an accurate account of it.
    _err_code = CAEN_DGTZ_GetRecordLength(handle, &_global_config.RecordLength);
    _print_if_err("CAEN_DGTZ_SetRecordLength", __FUNCTION__,
                  "reverting back to the provided record length");
    // if it fails, we write back the provided record length
    if (_err_code < 0) {
        _global_config.RecordLength = global_config.RecordLength;
    }

    // So far, all digitizer have the register 0x800C point to the
    // exponent of the number of buffers currently used.
    // TODO(Any): check if 0x800C is the register for all
    ReadRegister(0x800C, _current_max_buffers);
    _current_max_buffers = std::exp2(_current_max_buffers);

    _err_code = CAEN_DGTZ_SetPostTriggerSize(handle, _global_config.PostTriggerPorcentage);
    _print_if_err("CAEN_DGTZ_SetPostTriggerSize", __FUNCTION__);

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
    _err_code = CAEN_DGTZ_SetSWTriggerMode(handle, _global_config.SWTriggerMode);
    _print_if_err("CAEN_DGTZ_SetSWTriggerMode", __FUNCTION__);

    _err_code = CAEN_DGTZ_SetExtTriggerInputMode(handle, _global_config.EXTTriggerMode);
    _print_if_err("CAEN_DGTZ_SetExtTriggerInputMode", __FUNCTION__);

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
    _err_code = CAEN_DGTZ_SetAcquisitionMode(handle, _global_config.AcqMode);
    _print_if_err("CAEN_DGTZ_SetAcquisitionMode", __FUNCTION__);

    // Trigger polarity
    // These digitizers do not support channel-by-channel trigger pol
    // so we treat it like a global config, and use 0 as a placeholder.
    _err_code = CAEN_DGTZ_SetTriggerPolarity(handle, 0, _global_config.TriggerPolarity);
    _print_if_err("CAEN_DGTZ_SetTriggerPolarity", __FUNCTION__);

    _err_code = CAEN_DGTZ_SetIOLevel(handle, _global_config.IOLevel);
    _print_if_err("CAEN_DGTZ_SetIOLevel", __FUNCTION__);


    // Board config register
    // 0 = Trigger overlapping not allowed
    // 1 = trigger overlapping allowed
    WriteBits(0x8000, _global_config.TriggerOverlappingEn, 1);

    WriteBits(0x8100, _global_config.MemoryFullModeSelection, 5);

    // Channel stuff
    _group_configs = gr_configs;
    if (Family == CAENDigitizerFamilies::x730) {
        // For DT5730B, there are no groups only channels so we take
        // each configuration as a channel
        // First, we make the channel mask
        uint32_t channel_mask = 0;
        for (std::size_t ch = 0; ch < gr_configs.size(); ch++) {
            channel_mask |= _group_configs[ch].Enabled << ch;
        }

        uint32_t trg_mask = 0;
        for (std::size_t ch = 0; ch < gr_configs.size(); ch++) {
            trg_mask = _group_configs[ch].TriggerMask.get();
            bool has_trig_mask = trg_mask > 0;
            trg_mask |=  has_trig_mask << ch;
        }

        // Then enable those channels
        _err_code = CAEN_DGTZ_SetChannelEnableMask(handle, channel_mask);
        _print_if_err("CAEN_DGTZ_SetChannelEnableMask", __FUNCTION__);

        // Then enable if they are part of the trigger
        _err_code = CAEN_DGTZ_SetChannelSelfTrigger(handle,
                                                    _global_config.CHTriggerMode,
                                                    trg_mask);
        _print_if_err("CAEN_DGTZ_SetChannelSelfTrigger", __FUNCTION__);

        for (std::size_t ch = 0; ch < gr_configs.size(); ch++) {
            auto ch_config = gr_configs[ch];

            // Trigger stuff
            // Self Channel trigger
            _err_code = CAEN_DGTZ_SetChannelTriggerThreshold(handle,
                                                             ch,
                                                             ch_config.TriggerThreshold);
            _print_if_err("CAEN_DGTZ_SetChannelTriggerThreshold", __FUNCTION__);

            _err_code = CAEN_DGTZ_SetChannelDCOffset(handle, ch, ch_config.DCOffset);
            _print_if_err("CAEN_DGTZ_SetChannelDCOffset", __FUNCTION__);

            // Writes to the registers that holds the DC range
            // For 5730 it is the register 0x1n28
            WriteRegister(0x1028 | (ch & 0x0F) << 8, ch_config.DCRange & 0x0001);
        }

    } else if (Family == CAENDigitizerFamilies::x740) {
        uint32_t group_mask = 0;
        for (std::size_t grp_n = 0; grp_n < gr_configs.size(); grp_n++) {
            group_mask |= gr_configs[grp_n].Enabled << grp_n;
        }

        _err_code = CAEN_DGTZ_SetGroupEnableMask(handle, group_mask);
        _print_if_err("CAEN_DGTZ_SetGroupEnableMask", __FUNCTION__);

        _err_code = CAEN_DGTZ_SetGroupSelfTrigger(handle,
                                                  _global_config.CHTriggerMode,
                                                  group_mask);
        _print_if_err("CAEN_DGTZ_SetGroupSelfTrigger", __FUNCTION__);

        for (std::size_t grp_n = 0; grp_n < gr_configs.size(); grp_n++) {
            auto gr_config = gr_configs[grp_n];
            // Trigger stuff

            // This guy is does not work under V1740D unless in firmware
            // version 4.17
            _err_code = CAEN_DGTZ_SetGroupTriggerThreshold(handle,
                                                           grp_n,
                                                           gr_config.TriggerThreshold);
            _print_if_err("CAEN_DGTZ_SetGroupTriggerThreshold", __FUNCTION__);

            _err_code = CAEN_DGTZ_SetGroupDCOffset(handle,
                                                   grp_n,
                                                   gr_config.DCOffset);
            _print_if_err("CAEN_DGTZ_SetGroupDCOffset", __FUNCTION__);

            // Set the mask for channels enabled for self-triggering
            auto trig_mask = gr_config.TriggerMask.get();
            _err_code = CAEN_DGTZ_SetChannelGroupMask(handle,
                                                      grp_n,
                                                      trig_mask);
            _print_if_err("CAEN_DGTZ_SetChannelGroupMask", __FUNCTION__);

            // Set acquisition mask
            auto acq_mask = gr_config.AcquisitionMask.get();
            WriteBits(0x10A8 | (grp_n << 8),
                       acq_mask, 0, 8);

            // DCCorrections should be of length
            // NumberofChannels / NumberofGroups.
            // set individual channel 8-bitDC offset
            // on 12 bit LSB scale, same as threshold
            uint32_t word = 0;
            for (int ch = 0; ch < 4; ch++) {
                word += gr_config.DCCorrections[ch] << (ch * 8);
            }

            WriteBits(0x10C0 | (grp_n << 8), word);
            word = 0;
            for (int ch = 4; ch < 8; ch++) {
                word += gr_config.DCCorrections[ch] << ((ch - 4) * 8);
            }
            WriteBits(0x10C4 | (grp_n << 8), word);
        }

        bool trg_out = false;

        // For 740, to use TRG-IN as Gate / anti-veto
        if (_global_config.EXTAsGate) {
            WriteBits(0x810C, 1, 27);  // TRG-IN AND internal trigger,
            // and to serve as gate
            WriteBits(0x811C, 1, 10);  // TRG-IN as gate
        }

        if (trg_out) {
            WriteBits(0x811C, 0, 15);  // TRG-OUT based on internal signal
            WriteBits(0x811C, 0b00, 16, 2);  // TRG-OUT based
            // on internal signal
        }
        WriteBits( 0x811C, 0b01, 21, 2);

        // read_register(res, 0x8110, word);
        // word |= 1; // enable group 0 to participate in GPO
        // write_register(res, 0x8110, word);

    } else {
        // custom error message if not above models
        _err_code = CAEN_DGTZ_ErrorCode::CAEN_DGTZ_BadBoardType;
        _print_if_err("setup", __FUNCTION__,
                      "This API does not support your model/family."
                      " Maybe help writing the support code? :)");

    }
}

template<typename T, size_t N>
void CAEN<T, N>::EnableAcquisition() noexcept {
    if (_has_error or not _is_connected) {
        return;
    }

    int& handle = _caen_api_handle;

    // By using equals for CAENData and CAENEvent we release memory, too.
    // so by calling enable acquisition we also free memory and allocate mem.

    // We need a single data buffer to hold the incoming Data
    _caen_raw_data = CAENData{_logger, handle};
    _err_code = _caen_raw_data.getError();
    _print_if_err("CAENData", __FUNCTION__);

    // Allocates all the memory for the internal events buffer
    std::generate(_events.begin(), _events.end(), [](){
        return CAENEvent(handle);
    });

    _err_code = CAEN_DGTZ_ClearData(handle);
    _print_if_err("CAEN_DGTZ_ClearData", __FUNCTION__);

    _err_code = CAEN_DGTZ_SWStartAcquisition(handle);
    _print_if_err("CAEN_DGTZ_SWStartAcquisition", __FUNCTION__);

    if (not _has_error) {
        _is_acquiring = true;
    } else {
        // Try to disable the acquisition just in case
        _err_code = CAEN_DGTZ_SWStopAcquisition(_caen_api_handle);
        _print_if_err("CAEN_DGTZ_SWStopAcquisition",
                      __FUNCTION__);
        _is_acquiring = false;
    }
}

template<typename T, size_t N>
void CAEN<T, N>::DisableAcquisition() noexcept {
    if (_has_error or not _is_connected or not _is_acquiring) {
        return;
    }

    _err_code = CAEN_DGTZ_SWStopAcquisition(_caen_api_handle);
    _print_if_err("CAEN_DGTZ_SWStopAcquisition", __FUNCTION__);
    // To avoid false positives
    if (not _has_error) {
        _is_acquiring = false;
    }
}

template<typename T, size_t N>
void CAEN<T, N>::WriteRegister(uint32_t&& addr, const uint32_t& value) noexcept {
    if (_has_error or not _is_connected) {
        return;
    }

    _err_code = CAEN_DGTZ_WriteRegister(_caen_api_handle, addr, value);
    _print_if_err("CAEN_DGTZ_WriteRegister", __FUNCTION__);
}

template<typename T, size_t N>
void CAEN<T, N>::ReadRegister(uint32_t&& addr, uint32_t& value) noexcept {
    if (_has_error or not _is_connected) {
        return;
    }

    _err_code = CAEN_DGTZ_ReadRegister(_caen_api_handle, addr, &value);
    _print_if_err("CAEN_DGTZ_ReadRegister", __FUNCTION__);
}

template<typename T, size_t N>
void CAEN<T, N>::WriteBits(uint32_t&& addr,
    const uint32_t& value, uint8_t pos, uint8_t len) noexcept {
    if (_has_error or not _is_connected) {
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

template<typename T, size_t N>
void CAEN<T, N>::SoftwareTrigger() noexcept {
    if (_has_error or not _is_connected) {
        return;
    }

    _err_code = CAEN_DGTZ_SendSWtrigger(_caen_api_handle);
    _print_if_err("CAEN_DGTZ_SendSWtrigger", __FUNCTION__);
}

template<typename T, size_t N>
uint32_t CAEN<T, N>::GetEventsInBuffer() noexcept {
    uint32_t events = 0;
    // For 5730 it is the register 0x812C
    // TODO(Any): expand to include any registers depending on the model
    // or family.
    ReadRegister(0x812C, events);

    return events;
}

template<typename T, size_t N>
void CAEN<T, N>::RetrieveData() noexcept {
    int& handle = _caen_api_handle;

    if (_has_error or not _is_connected or not _is_acquiring) {
        return;
    }

    // UNSAFE CODE AHEAD
    _err_code = CAEN_DGTZ_ReadData(handle,
        CAEN_DGTZ_ReadMode_t::CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,
        _caen_raw_data.Buffer,
        _caen_raw_data.DataSize);
    _print_if_err("CAEN_DGTZ_ReadData", __FUNCTION__);

    _err_code = CAEN_DGTZ_GetNumEvents(handle,
                                       _caen_raw_data.Buffer,
                                       _caen_raw_data.DataSize,
                                       &_caen_raw_data.NumEvents);
    // END OF UNSAFE CODE
    _print_if_err("CAEN_DGTZ_GetNumEvents", __FUNCTION__);
}

template<typename T, size_t N>
bool CAEN<T, N>::RetrieveDataUntilNEvents(uint32_t& n, bool debug_info) noexcept {
    int& handle = _caen_api_handle;

    if (_has_error or not _is_connected or not _is_acquiring) {
        return false;
    }

    if (n >= _current_max_buffers) {
        if (GetEventsInBuffer() < _current_max_buffers) {
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

    RetrieveData();

    return true;
}

template<typename T, size_t N>
const CAENEvent* CAEN<T, N>::DecodeEvent(const uint32_t& i) noexcept {
    if (i > _caen_raw_data.NumEvents) {
        return &_events[_caen_raw_data.NumEvents - 1];
    }

    if (_has_error or not _is_connected) {
        return nullptr;
    }

    _err_code = _events[i].getEventInfo(_caen_raw_data.Buffer,
                                        _caen_raw_data.DataSize,
                                        i);
    _print_if_err("CAEN_DGTZ_GetEventInfo",
                  __FUNCTION__,
                  "at event " + std::to_string(i));
    // Cannot decode without getting event info
    _err_code = _events[i].decodeEvent();
    _print_if_err("CAEN_DGTZ_DecodeEvent",
                  __FUNCTION__,
                  "at event " + std::to_string(i));

    return &_events[i];
}

template<typename T, size_t N>
void CAEN<T, N>::DecodeEvents() noexcept {
    if (_has_error or not _is_connected) {
        return;
    }

    for (uint32_t i = 0; i < _caen_raw_data.NumEvents; i++) {
        _err_code = _events[i].getEventInfo(_caen_raw_data.Buffer,
                                            _caen_raw_data.DataSize,
                                            i);
        _print_if_err("CAEN_DGTZ_GetEventInfo",
                      __FUNCTION__,
                      "at event " + std::to_string(i));
        // Cannot decode without getting event info
        _err_code = _events[i].decodeEvent();
        _print_if_err("CAEN_DGTZ_DecodeEvent",
                      __FUNCTION__,
                      "at event " + std::to_string(i));
    }
}

template<typename T, size_t N>
void CAEN<T, N>::ClearData() noexcept {
    if (_has_error or not _is_connected) {
        return;
    }

    int& handle = _caen_api_handle;
    _err_code = CAEN_DGTZ_SWStopAcquisition(handle);
    _print_if_err("CAEN_DGTZ_SWStopAcquisition", __FUNCTION__);
    _err_code = CAEN_DGTZ_ClearData(handle);
    _print_if_err("CAEN_DGTZ_ClearData", __FUNCTION__);
    _err_code = CAEN_DGTZ_SWStartAcquisition(handle);
    _print_if_err("CAEN_DGTZ_SWStartAcquisition", __FUNCTION__);
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
//std::string sbc_init_file(CAEN&) noexcept;
//
//// Saves the digitizer data in the Binary format SBC collboration is using
//std::string sbc_save_func(CAENEvent&, CAEN&) noexcept;

/// End File functions

}   // namespace SBCQueens
#endif
