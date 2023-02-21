#include "sbcqueens-gui/caen_helper.hpp"

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <CAENDigitizer.h>
#include <cmath>
#include <cstdint>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <bitset>
#include <iostream>
#include <functional>

// C++ 3rd party includes
#include <CAENComm.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

// my includes

namespace SBCQueens {

std::string translate_caen_error_code(const CAEN_DGTZ_ErrorCode& err) {
    switch(err) {
        case CAEN_DGTZ_Success:
            return "Success";
        case CAEN_DGTZ_CommError:
            return "Communication error";
        default:
        case CAEN_DGTZ_GenericError:
            return "Unspecified error";
        case CAEN_DGTZ_InvalidParam:
            return "Invalid parameter";
        case CAEN_DGTZ_InvalidLinkType:
            return "Invalid link type";
        case CAEN_DGTZ_InvalidHandle:
            return "Invalid device handle";
        case CAEN_DGTZ_MaxDevicesError:
            return "Maximum number of devices exceeded";
        case CAEN_DGTZ_BadBoardType:
            return "The operation is not allowed on this type of board";
        case CAEN_DGTZ_BadInterruptLev:
            return "The interrupt level is not allowed";
        case CAEN_DGTZ_BadEventNumber:
            return "The event number is bad ";
        case CAEN_DGTZ_ReadDeviceRegisterFail:
            return "Unable to read the registry";
        case CAEN_DGTZ_WriteDeviceRegisterFail:
            return "Unable to write into the registry";
        case CAEN_DGTZ_InvalidChannelNumber:
            return "The channel number is invalid";
        case CAEN_DGTZ_ChannelBusy:
            return "The Channel is busy";
        case CAEN_DGTZ_FPIOModeInvalid:
            return "Invalid FPIO Mode";
        case CAEN_DGTZ_WrongAcqMode:
            return "Wrong acquisition mode";
        case CAEN_DGTZ_FunctionNotAllowed:
            return "This function is not allowed for this module";
        case CAEN_DGTZ_Timeout:
            return "Communication Timeout";
        case CAEN_DGTZ_InvalidBuffer:
            return "The buffer is invalid";
        case CAEN_DGTZ_EventNotFound:
            return "The event is not found";
        case CAEN_DGTZ_InvalidEvent:
            return "The event is invalid";
        case CAEN_DGTZ_OutOfMemory:
            return "Out of memory";
        case CAEN_DGTZ_CalibrationError:
            return "Unable to calibrate the board";
        case CAEN_DGTZ_DigitizerNotFound:
            return "Unable to open the digitizer";
        case CAEN_DGTZ_DigitizerAlreadyOpen:
            return "The Digitizer is already open";
        case CAEN_DGTZ_DigitizerNotReady:
            return "The Digitizer is not ready to operate";
        case CAEN_DGTZ_InterruptNotConfigured:
            return "The Digitizer has not the IRQ configured";
        case CAEN_DGTZ_DigitizerMemoryCorrupted:
            return "The digitizer flash memory is corrupted";
        case CAEN_DGTZ_DPPFirmwareNotSupported:
            return "The digitizer dpp firmware is not supported in this lib version";
        case CAEN_DGTZ_InvalidLicense:
            return "Invalid Firmware License";
        case CAEN_DGTZ_InvalidDigitizerStatus:
            return "The digitizer is found in a corrupted status";
        case CAEN_DGTZ_UnsupportedTrace:
            return "The given trace is not supported by the digitizer";
        case CAEN_DGTZ_InvalidProbe:
            return "The given probe is not supported for the given digitizer's trace";
        case CAEN_DGTZ_UnsupportedBaseAddress:
            return "The Base Address is not supported, it's a Desktop device?";
        case CAEN_DGTZ_NotYetImplemented:
            return "The function is not yet implemented";
    }
}
//
//CAENError connect(CAEN& res, const CAENDigitizerModel& model,
//    const CAENConnectionType& ct,
//    const int& ln, const int& cn, const uint32_t& addr) noexcept {
//    CAENError err;
//
//    // If the item exists, pass on the creation
//    if (res) {
//        err = CAENError {
//            "Resource already connected",  // ErrorMessage
//            CAEN_DGTZ_ErrorCode::CAEN_DGTZ_GenericError,  // ErrorCode
//            true  // isError
//        };
//
//        return err;
//    }
//
//    int handle = 0;
//    int errCode;
//    switch (ct) {
//        case CAENConnectionType::A4818:
//
//            errCode = CAEN_DGTZ_OpenDigitizer(
//                CAEN_DGTZ_ConnectionType::CAEN_DGTZ_USB_A4818,
//                ln, 0, addr, &handle);
//
//        break;
//
//        case CAENConnectionType::USB:
//        default:
//            errCode = CAEN_DGTZ_OpenDigitizer(
//                CAEN_DGTZ_ConnectionType::CAEN_DGTZ_USB,
//                ln, cn, 0, 0);
//        break;
//    }
//    // errCode = CAEN_DGTZ_OpenDigitizer(ct, ln, cn, addr, &handle);
//
//    if (errCode >= 0) {
//        res = std::make_unique<caen>(model, ct, ln, cn, addr,
//            handle, err);
//    } else {
//        return CAENError {
//            "Failed to connect to the resource",  // ErrorMessage
//            static_cast<CAEN_DGTZ_ErrorCode>(errCode),  // ErrorCode
//            true   // isError
//        };
//    }
//
//    return CAENError();
//}
//
//// CAENError connect_usb(CAEN& res, const CAENDigitizerModel& model,
////  const int& linkNum) noexcept {
////  return connect(res, model, CAEN_DGTZ_ConnectionType::CAEN_DGTZ_USB,
////    linkNum, 0, 0);
//// }
//
//CAENError disconnect(CAEN& res) noexcept {
//    if (!res) {
//        return CAENError();
//    }
//
//    int& handle = res->Handle;
//    // Before disconnecting, stop acquisition, and clear data
//    // (clear data should only be called after the acquisition has been
//    // stopped)
//    // Resources are freed when the pointer is reset
//    int errCode = CAEN_DGTZ_SWStopAcquisition(handle);
//
//    errCode |= CAEN_DGTZ_FreeReadoutBuffer(&res->Data.Buffer);
//    // errCode |= CAEN_DGTZ_ClearData(handle);
//    errCode |= CAEN_DGTZ_CloseDigitizer(handle);
//
//    if (errCode < 0) {
//        return CAENError {
//            "Error while disconnecting CAEN resource. "
//            "Will delete internal resource anyways.",  // ErrorMessage
//            static_cast<CAEN_DGTZ_ErrorCode>(errCode),  // ErrorCode
//            true  // isError
//        };
//    }
//
//    res.reset();
//    return CAENError();
//}
//
//void reset(CAEN& res) noexcept {
//    if (!res) {
//        return;
//    }
//
//    auto err = CAEN_DGTZ_Reset(res->Handle);
//
//    if (err < 0) {
//        res->LatestError = CAENError {
//            "Could not reset CAEN board.",  // ErrorMessage
//            err,  // ErrorCode
//            true  // isError
//        };
//    }
//}
//
//void calibrate(CAEN& res) noexcept {
//    if (!res) {
//        return;
//    }
//
//    if (res->LatestError.isError) {
//        return;
//    }
//
//    // int& handle = res->Handle;
//}
//
//void setup(CAEN &res, CAENGlobalConfig g_config,
//           const std::array<CAENGroupConfig, 8>& gr_configs) noexcept {
//    if (!res) {
//        return;
//    }
//
//    if (res->LatestError.isError) {
//        return;
//    }
//
//    reset(res);
//    int &handle = res->Handle;
//    int latest_err = 0;
//    std::string err_msg = "";
//
//    // This lambda wraps whatever CAEN function you pass to it,
//    // checks the error, and if there was an error, add the error msg to
//    // it. If there is an error, it also does not execute the function.
//    auto error_wrap = [&](std::string msg, auto f, auto... args) {
//        if (latest_err >= 0) {
//            auto err = f(args...);
//            if (err < 0) {
//                spdlog::error("caen error {}, {}", err, msg);
//                latest_err = err;
//                err_msg += msg;
//            }
//        }
//    };
//
//    error_wrap("CAEN_DGTZ_GetInfo Failed. ",
//                         CAEN_DGTZ_GetInfo, handle,
//                         &res->CAENBoardInfo);
//
//    // Global config
//    res->GlobalConfig = g_config;
//    error_wrap("CAEN_DGTZ_SetMaxNumEventsBLT Failed. ",
//                         CAEN_DGTZ_SetMaxNumEventsBLT, handle,
//                         res->GlobalConfig.MaxEventsPerRead);
//    // Before:
//    // err = CAEN_DGTZ_SetMaxNumEventsBLT(handle,
//    //  res->GlobalConfig.MaxEventsPerRead);
//    // if(err < 0) {
//    //    total_err |= err;
//    //    err_msg += "CAEN_DGTZ_SetMaxNumEventsBLT Failed. ";
//    // }
//
//    error_wrap("CAEN_DGTZ_SetRecordLength Failed. ",
//                         CAEN_DGTZ_SetRecordLength, handle,
//                         res->GlobalConfig.RecordLength);
//
//    // We get the actual record length
//    CAEN_DGTZ_GetRecordLength(handle, &res->GlobalConfig.RecordLength);
//
//    // This will calculate what is the max current max buffers
//    // res->CurrentMaxBuffers = calculate_max_buffers(res);
//    read_register(res, 0x800C, res->CurrentMaxBuffers);
//    res->CurrentMaxBuffers = std::exp2(res->CurrentMaxBuffers);
//
//    spdlog::info("Record length = {}, and buffers = {}",
//        res->GlobalConfig.RecordLength,
//        res->CurrentMaxBuffers);
//
//    // Failing on x1740
//    // Worked after updating firmware to 4.17
//    error_wrap("CAEN_DGTZ_SetPostTriggerSize Failed. ",
//                         CAEN_DGTZ_SetPostTriggerSize, handle,
//                         res->GlobalConfig.PostTriggerPorcentage);
//
//    // Trigger Mode comes in four flavors:
//    // CAEN_DGTZ_TRGMODE_DISABLED
//    //    is disabled
//    // CAEN_DGTZ_TRGMODE_EXTOUT_ONLY
//    //    is used to generate the trigger output
//    // CAEN_DGTZ_TRGMODE_ACQ_ONLY
//    //    is used to generate the acquisition trigger
//    // CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT
//    //    is used to generate the acquisition trigger and trigger
//    // output
//    error_wrap("CAEN_DGTZ_SetSWTriggerMode Failed. ",
//                         CAEN_DGTZ_SetSWTriggerMode, handle,
//                         res->GlobalConfig.SWTriggerMode);
//
//    error_wrap("CAEN_DGTZ_SetExtTriggerInputMode Failed. ",
//                         CAEN_DGTZ_SetExtTriggerInputMode, handle,
//                         res->GlobalConfig.EXTTriggerMode);
//
//    // Acquisition mode comes in four flavors:
//    // CAEN_DGTZ_SW_CONTROLLED
//    //    Start/stop of the run takes place on software
//    //    command (by calling CAEN_DGTZ_SWStartAcquisition )
//    // CAEN_DGTZ_FIRST_TRG_CONTROLLED
//    //    Run starts on the first trigger pulse (rising edge on
//    // TRG-IN)    actual triggers start from the second pulse from there
//    // CAEN_DGTZ_S_IN_CONTROLLED
//    //    S-IN/GPI controller (depends on the board). Acquisition
//    //    starts on edge of the GPI/S-IN
//    // CAEN_DGTZ_LVDS_CONTROLLED
//    //    VME ONLY, like S_IN_CONTROLLER but uses LVDS.
//    error_wrap("CAEN_DGTZ_SetAcquisitionMode Failed. ",
//                         CAEN_DGTZ_SetAcquisitionMode, handle,
//                         res->GlobalConfig.AcqMode);
//
//    // Trigger polarity
//    error_wrap("CAEN_DGTZ_SetTriggerPolarity Failed. ",
//                         CAEN_DGTZ_SetTriggerPolarity, handle, 0,
//                         res->GlobalConfig.TriggerPolarity);
//
//    // IO Level
//    error_wrap("CAEN_DGTZ_SetIOLevel Failed. ",
//                         CAEN_DGTZ_SetIOLevel, handle,
//                         res->GlobalConfig.IOLevel);
//
//    // Board config register
//    // 0 = Trigger overlapping not allowed
//    // 1 = trigger overlapping allowed
//    write_bits(res, 0x8000, res->GlobalConfig.TriggerOverlappingEn, 1);
//
//    write_bits(res, 0x8100, res->GlobalConfig.MemoryFullModeSelection, 5);
//
//    // Channel stuff
//    res->GroupConfigs = gr_configs;
//    if (res->Family == CAENDigitizerFamilies::x730) {
//        // For DT5730B, there are no groups only channels so we take
//        // each configuration as a channel
//        // First, we make the channel mask
//        uint32_t channel_mask = 0;
//        for (std::size_t ch = 0; ch < gr_configs.size(); ch++) {
//            channel_mask |= res->GroupConfigs[ch].Enabled << ch;
//        }
//
//        uint32_t trg_mask = 0;
//        for (std::size_t ch = 0; ch < gr_configs.size(); ch++) {
//            trg_mask = res->GroupConfigs[ch].TriggerMask.get();
//            bool has_trig_mask = trg_mask > 0;
//            trg_mask |=  has_trig_mask << ch;
//        }
//
//        // Then enable those channels
//        error_wrap("CAEN_DGTZ_SetChannelEnableMask Failed. ",
//                             CAEN_DGTZ_SetChannelEnableMask,
//                             handle, channel_mask);
//
//        // Then enable if they are part of the trigger
//        error_wrap("CAEN_DGTZ_SetChannelSelfTrigger Failed. ",
//                             CAEN_DGTZ_SetChannelSelfTrigger, handle,
//                             res->GlobalConfig.CHTriggerMode, trg_mask);
//
//        for (std::size_t ch = 0; ch < gr_configs.size(); ch++) {
//            auto ch_config = gr_configs[ch];
//
//            // Trigger stuff
//            // Self Channel trigger
//            error_wrap("CAEN_DGTZ_SetChannelTriggerThreshold Failed. ",
//                CAEN_DGTZ_SetChannelTriggerThreshold, handle,
//                ch, ch_config.TriggerThreshold);
//
//            error_wrap("CAEN_DGTZ_SetChannelDCOffset Failed. ",
//                CAEN_DGTZ_SetChannelDCOffset, handle, ch,
//                ch_config.DCOffset);
//
//            // Writes to the registers that holds the DC range
//            // For 5730 it is the register 0x1n28
//            error_wrap("write_register to reg 0x1n28 Failed (DC Range). ",
//                                 CAEN_DGTZ_WriteRegister, handle,
//                                 0x1028 | (ch & 0x0F) << 8,
//                                 ch_config.DCRange & 0x0001);
//        }
//
//    } else if (res->Family == CAENDigitizerFamilies::x740) {
//        uint32_t group_mask = 0;
//        for (std::size_t grp_n = 0; grp_n < gr_configs.size(); grp_n++) {
//            group_mask |= gr_configs[grp_n].Enabled << grp_n;
//        }
//
//        error_wrap("CAEN_DGTZ_SetGroupEnableMask Failed. ",
//                   CAEN_DGTZ_SetGroupEnableMask, handle, group_mask);
//
//        error_wrap("CAEN_DGTZ_SetGroupSelfTrigger Failed. ",
//                             CAEN_DGTZ_SetGroupSelfTrigger, handle,
//                             res->GlobalConfig.CHTriggerMode, group_mask);
//
//        for (std::size_t grp_n = 0; grp_n < gr_configs.size(); grp_n++) {
//            auto gr_config = gr_configs[grp_n];
//            // Trigger stuff
//
//            // This guy is does not work under V1740D unless in firmware
//            // version 4.17
//            error_wrap("CAEN_DGTZ_SetGroupTriggerThreshold Failed. ",
//                       CAEN_DGTZ_SetGroupTriggerThreshold, handle,
//                       grp_n, gr_config.TriggerThreshold);
//
//            error_wrap("CAEN_DGTZ_SetGroupDCOffset Failed. ",
//                       CAEN_DGTZ_SetGroupDCOffset,
//                       handle, grp_n,
//                       gr_config.DCOffset);
//
//            // Set the mask for channels enabled for self-triggering
//            auto trig_mask = gr_config.TriggerMask.get();
//            error_wrap("CAEN_DGTZ_SetChannelGroupMask Failed. ",
//                CAEN_DGTZ_SetChannelGroupMask,
//                handle, grp_n, trig_mask);
//
//            // Set acquisition mask
//            auto acq_mask = gr_config.AcquisitionMask.get();
//            write_bits(res, 0x10A8 | (grp_n << 8),
//                acq_mask, 0, 8);
//
//            // DCCorrections should be of length
//            // NumberofChannels / NumberofGroups.
//            // set individual channel 8-bitDC offset
//            // on 12 bit LSB scale, same as threshold
//            uint32_t word = 0;
//            for (int ch = 0; ch < 4; ch++) {
//                word += gr_config.DCCorrections[ch] << (ch * 8);
//            }
//            write_register(res, 0x10C0 | (grp_n << 8), word);
//            word = 0;
//            for (int ch = 4; ch < 8; ch++) {
//                word += gr_config.DCCorrections[ch] << ((ch - 4) * 8);
//            }
//            write_register(res, 0x10C4 | (grp_n << 8), word);
//        }
//
//
//        bool trg_out = false;
//
//        // For 740, to use TRG-IN as Gate / anti-veto
//        if (g_config.EXTasGate) {
//            write_bits(res, 0x810C, 1, 27);  // TRG-IN AND internal trigger,
//            // and to serve as gate
//            write_bits(res, 0x811C, 1, 10);  // TRG-IN as gate
//        }
//
//        if (trg_out) {
//            write_bits(res, 0x811C, 0, 15);  // TRG-OUT based on internal signal
//            write_bits(res, 0x811C, 0b00, 16, 2);  // TRG-OUT based
//            // on internal signal
//        }
//        write_bits(res, 0x811C, 0b01, 21, 2);
//
//        // read_register(res, 0x8110, word);
//        // word |= 1; // enable group 0 to participate in GPO
//        // write_register(res, 0x8110, word);
//
//    } else {
//        // custom error message if not above models
//        latest_err = -1;
//        err_msg += "Model family not supported.";
//    }
//
//    if (latest_err < 0) {
//        res->LatestError = CAENError {
//                "There was en error during setup! " + err_msg,  // ErrorMessage
//                static_cast<CAEN_DGTZ_ErrorCode>(latest_err),  // ErrorCode
//                true};  // isError
//    }
//}
//
//void enable_acquisition(CAEN& res) noexcept {
//    if (!res) {
//        return;
//    }
//
//    if (res->LatestError.isError) {
//        return;
//    }
//
//    int& handle = res->Handle;
//
//    int err = CAEN_DGTZ_MallocReadoutBuffer(handle,
//        &res->Data.Buffer, &res->Data.TotalSizeBuffer);
//
//    err |= CAEN_DGTZ_ClearData(handle);
//    err |= CAEN_DGTZ_SWStartAcquisition(handle);
//
//    if (err < 0) {
//        res->LatestError = CAENError {
//            "There was en error while trying to enable"
//            "acquisition or allocate memory for buffers.",  // ErrorMessage
//            static_cast<CAEN_DGTZ_ErrorCode>(err),  // ErrorCode
//            true  // isError
//        };
//    }
//}
//
//void disable_acquisition(CAEN& res) noexcept {
//    if (!res) {
//        return;
//    }
//
//    auto err = CAEN_DGTZ_SWStopAcquisition(res->Handle);
//
//    if (err < 0) {
//        res->LatestError = CAENError {
//            "There was en error while trying to disable"
//            "acquisition.",  // ErrorMessage
//            err,  // ErrorCode
//            true  // isError
//        };
//    }
//}
//
//void write_register(CAEN& res, uint32_t&& addr,
//    const uint32_t& value) noexcept {
//    if (!res) {
//        return;
//    }
//
//    if (res->LatestError.isError) {
//        return;
//    }
//
//    auto err = CAEN_DGTZ_WriteRegister(res->Handle, addr, value);
//    if (err < 0) {
//        res->LatestError = CAENError {
//            "There was en error while trying to write "
//            "to register.",  // ErrorMessage
//            err,  // ErrorCode
//            true  // isError
//        };
//    }
//}
//
//void read_register(CAEN& res, uint32_t&& addr, uint32_t& value) noexcept {
//    if (!res) {
//        return;
//    }
//
//    auto err = CAEN_DGTZ_ReadRegister(res->Handle, addr, &value);
//    if (err < 0) {
//        res->LatestError = CAENError {
//            "There was en error while trying to read"
//            "to register " + std::to_string(addr),  // ErrorMessage
//            err,  // ErrorCode
//            true  // isError
//        };
//    }
//}
//
//void write_bits(CAEN& res, uint32_t&& addr,
//    const uint32_t& value, uint8_t pos, uint8_t len) noexcept {
//    if (!res) {
//        return;
//    }
//
//    // First read the register
//    uint32_t read_word = 0;
//    auto err = CAEN_DGTZ_ReadRegister(res->Handle, addr, &read_word);
//
//    if (err < 0) {
//        res->LatestError = CAENError {
//            "There was en error while trying to read"
//            "to register " + std::to_string(addr),  // ErrorMessage
//            err,  // ErrorCode
//            true  // isError
//        };
//    }
//
//    uint32_t bit_mask = ~(((1 << len) - 1) << pos);
//    read_word = read_word & bit_mask; //mask the register value
//
//    // Get the lowest bits of value and shifted to the correct position
//    uint32_t value_bits = (value & ((1 << len) - 1)) << pos;
//    // Combine masked value read from register with new bits
//    err = CAEN_DGTZ_WriteRegister(res->Handle, addr, read_word | value_bits);
//
//    if (err < 0) {
//        res->LatestError = CAENError {
//            "There was en error while trying to write"
//            "to register " + std::to_string(addr),  // ErrorMessage
//            err,  // ErrorCode
//            true  // isError
//        };
//    }
//}
//
//void software_trigger(CAEN& res) noexcept {
//    if (!res) {
//        return;
//    }
//
//    if (res->LatestError.isError) {
//        return;
//    }
//
//
//    // auto err = CAEN_DGTZ_SendSWtrigger(res->Handle);
//    write_register(res, 0x8108, 420);
//
//    // if (err < 0) {
//    //     res->LatestError = CAENError {
//    //         "Failed to send a software trigger",  // ErrorMessage
//    //         err,  // ErrorCode
//    //         true  // isError
//    //     };
//    // }
//}
//
//uint32_t get_events_in_buffer(CAEN& res) noexcept {
//    uint32_t events = 0;
//    // For 5730 it is the register 0x812C
//    read_register(res, 0x812C, events);
//
//    return events;
//}
//
//void retrieve_data(CAEN& res) noexcept {
//    int& handle = res->Handle;
//
//    if (!res) {
//        return;
//    }
//
//    if (res->LatestError.ErrorCode < 0) {
//        return;
//    }
//
//    int err = CAEN_DGTZ_ReadData(handle,
//        CAEN_DGTZ_ReadMode_t::CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,
//        res->Data.Buffer,
//        &res->Data.DataSize);
//
//    if (err >= 0) {
//        err = CAEN_DGTZ_GetNumEvents(handle,
//            res->Data.Buffer,
//            res->Data.DataSize,
//            &res->Data.NumEvents);
//    }
//
//    if (err < 0) {
//        res->LatestError = CAENError {
//            "Failed to retrieve data!",  // ErrorMessage
//            static_cast<CAEN_DGTZ_ErrorCode>(err),  // ErrorCode
//            true  // isError
//        };
//    }
//}
//
//bool retrieve_data_until_n_events(CAEN& res, uint32_t& n,
//    bool debug_info) noexcept {
//    int& handle = res->Handle;
//
//    if (!res->start_rate_calculation) {
//        res->ts = std::chrono::high_resolution_clock::now();
//        res->start_rate_calculation = true;
//        // When starting statistics mode, mark time
//    }
//
//    if (!res) {
//        return false;
//    }
//
//    if (n >= res->CurrentMaxBuffers) {
//        if (get_events_in_buffer(res) < res->CurrentMaxBuffers) {
//            return false;
//        }
//    } else if (get_events_in_buffer(res) < n) {
//        return false;
//    }
//
//
//    if (res->LatestError.ErrorCode < 0) {
//        return false;
//    }
//
//    // There is a weird quirk related to this function that BLOCKS
//    // the software essentially killing it.
//    // If TriggerOverlapping is allowed sometimes this function
//    // returns more data than Buffer it can hold, why? Idk
//    // but so far with the software as is, it won't work with that
//    // so dont do it!
//    res->LatestError.ErrorCode = CAEN_DGTZ_ReadData(handle,
//        CAEN_DGTZ_ReadMode_t::CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,
//        res->Data.Buffer,
//        &res->Data.DataSize);
//
//    if (res->LatestError.ErrorCode < 0){
//        res->LatestError.isError = true;
//        res->LatestError.ErrorMessage = "There was an error when trying "
//        "to read buffer";
//        return false;
//    }
//
//    res->LatestError.ErrorCode = CAEN_DGTZ_GetNumEvents(handle,
//            res->Data.Buffer,
//            res->Data.DataSize,
//            &res->Data.NumEvents);
//
//    // Calculate trigger rate
//    res->te = std::chrono::high_resolution_clock::now();
//    res->t_us = std::chrono::
//        duration_cast<std::chrono::microseconds>(res->te - res->ts).count();
//    res->n_events = res->Data.NumEvents;
//    res->ts = res->te;
//    res->duration += res->t_us;
//    res->trg_count += res->n_events;
//
//    if (debug_info){
//        spdlog::info(fmt::format("#Triggers: {:7d}, Duration: {:-6.2f}s, Inst Rate:{:-8.2f}Hz, Accl Rate:{:-8.2f}Hz",
//            res->trg_count,
//            res->duration/1e6,
//            res->n_events*1e6/res->t_us,
//            res->trg_count*1e6/res->duration));
//    }
//
//    if (res->LatestError.ErrorCode < 0){
//        res->LatestError.isError = true;
//        res->LatestError.ErrorMessage = "There was an error when trying"
//        "to recover number of events from buffer";
//        return false;
//    }
//
//    return true;
//}
//
//void extract_event(CAEN& res, const uint32_t& i, CAENEvent& evt) noexcept {
//    int& handle = res->Handle;
//
//    if (!res ) {
//        return;
//    }
//
//    if (res->LatestError.ErrorCode < 0 ){
//        return;
//    }
//
//    if (!evt) {
//        evt = std::make_shared<caenEvent>(handle);
//    }
//
//    CAEN_DGTZ_GetEventInfo(handle,
//        res->Data.Buffer,
//        res->Data.DataSize,
//        i,
//        &evt->Info,
//        &evt->DataPtr);
//
//    CAEN_DGTZ_DecodeEvent(handle,
//        evt->DataPtr,
//        reinterpret_cast<void**>(&evt->Data));
//}
//
//void clear_data(CAEN& res) noexcept {
//    if (!res) {
//        return;
//    }
//
//    int& handle = res->Handle;
//    int err = CAEN_DGTZ_SWStopAcquisition(handle);
//    err |= CAEN_DGTZ_ClearData(handle);
//    err |= CAEN_DGTZ_SWStartAcquisition(handle);
//    if (err < 0) {
//        res->LatestError = CAENError {
//            "Failed to send a software trigger",  // ErrorMessage
//            static_cast<CAEN_DGTZ_ErrorCode>(err),  // ErrorCode
//            true  // isError
//        };
//    }
//}
//
//// uint32_t channel_self_trigger_meter(CAEN& res, uint8_t channel) noexcept {
////  uint32_t freq = 0;
////  // For 5730 it is 0x1nEC where n is the channel
////  read_register(res, 0x10EC | (channel & 0x0F) << 8,
////           freq);
//
////  return freq;
//// }
//
//uint32_t t_to_record_length(CAEN& res, const double& nsTime) noexcept {
//    return static_cast<uint32_t>(nsTime*1e-9*res->GetCommTransferRate());
//}
//
//uint32_t v_threshold_cts_to_adc_cts(const CAEN& res, const uint32_t& cts) noexcept {
//    const uint32_t kVthresholdRangeCts = std::exp2(16);
//    uint32_t ADCRange = std::exp2(res->ModelConstants.ADCResolution);
//
//    uint32_t out = ADCRange*cts;
//    return out / kVthresholdRangeCts;
//}
//
//// Turns a voltage (V) into trigger counts
//uint32_t v_to_threshold_counts(const double& volt) noexcept {
//    const uint32_t kVthresholdRangeCts = std::exp2(16);
//    return static_cast<uint32_t>(volt / kVthresholdRangeCts);
//}
//
//// Turns a voltage (V) into count offset
//uint32_t v_offset_to_count_offset(const CAEN& res, const double& volt) noexcept {
//    uint32_t ADCRange = std::exp2(res->ModelConstants.ADCResolution);
//    return static_cast<uint32_t>(volt / ADCRange);
//}
//
//// Calculates the number of buffers that are going to be used
//// given current settings.
//uint32_t calculate_max_buffers(CAEN& res) noexcept {
//    // Instead of using the stored RecordLength
//    // We will let CAEN functions do all the hard work for us
//    // uint32_t rl = res->GlobalConfig.RecordLength;
//    uint32_t rl = res->GlobalConfig.RecordLength;
//
//    // Cannot be higher than the memory per channel
//    auto mem_per_ch = res->ModelConstants.MemoryPerChannel;
//    uint32_t nloc = 0;
//
//    // This contains nloc which if multiplied by the correct
//    // constant, will give us the record length exactly
//    // read_register(res, 0x8020, nloc);
//    // spdlog::info("read an nloc = {0}", nloc);
//    // rl =  res->ModelConstants.NLOCToRecordLength*nloc;
//    // Then we calculate the actual num buffs but...
//    auto max_num_buffs = mem_per_ch / rl;
//
//    // It cannot be higher than the max buffers
//    max_num_buffs = max_num_buffs >= res->ModelConstants.MaxNumBuffers ?
//        res->ModelConstants.MaxNumBuffers : max_num_buffs;
//
//    // It can only be a power of 2
//    uint32_t real_max_buffs
//        = static_cast<uint32_t>(exp2(floor(log2(max_num_buffs))));
//
//    // Unless this is enabled, then it is always that number minus one.
//    // Except when the buffers is 1
//    if (res->GlobalConfig.MemoryFullModeSelection) {
//        if (real_max_buffs > 2) {
//            real_max_buffs--;
//        } else {
//            real_max_buffs = 1;
//        }
//    }
//
//    spdlog::info("Number of buffers {0}", real_max_buffs);
//
//    // Phew, we are done!
//    return real_max_buffs;
//}

// std::string sbc_init_file(CAEN& res) noexcept {
//     // header string = name;type;x,y,z...;
//     auto g_config = res->GlobalConfig;
//     auto group_configs = res->GroupConfigs;
//     const uint8_t num_groups = res->ModelConstants.NumberOfGroups;
//     const bool has_groups = num_groups > 0;

//     // The enum and the map is to simplify the code for the lambdas.
//     // and the header could potentially change any moment with any type
//     enum class Types { CHAR, INT8, INT16, INT32, INT64, UINT8, UINT16,
//         UINT32, UINT64, SINGLE, FLOAT, DOUBLE, FLOAT128 };

//     const std::unordered_map<Types, std::string> type_to_string = {
//         {Types::CHAR, "char"},
//         {Types::INT8, "int8"}, {Types::INT16, "int16"},
//         {Types::INT32, "int32"}, {Types::UINT8, "uint8"},
//         {Types::UINT16, "uint16"}, {Types::UINT32, "uint32"},
//         {Types::SINGLE, "single"}, {Types::FLOAT, "single"},
//         {Types::DOUBLE, "double"}, {Types::FLOAT128, "float128"}
//     };

//     auto NumToBinString = [] (auto num) -> std::string {
//         char* tmpstr = reinterpret_cast<char*>(&num);
//         return std::string(tmpstr, sizeof(num) / sizeof(char));
//     };

//     // Lambda to help to save a single number to the binary format
//     // file. The numbers are always converted to double
//     auto SingleDimToBinaryHeader = [&] (std::string name, Types type,
//         uint32_t length) -> std::string {
//         auto type_s = type_to_string.at(type);
//         auto header = name + ";" + type_s + ";" + std::to_string(length) + ";";

//         return header;
//     };

//     uint32_t l = 0x01020304;
//     auto endianness_s = NumToBinString(l);

//     std::string header = SingleDimToBinaryHeader("sample_rate",
//         Types::DOUBLE, 1);

//     std::function<uint32_t(uint32_t)> n_channels_acq = [&](uint8_t acq_mask) {
//         return acq_mask == 0 ? 0 : (acq_mask & 1) + n_channels_acq(acq_mask>>1);
//     };

//     uint32_t num_ch = 0;
//     for (auto gr_pair : res->GroupConfigs) {
//         // This bool will make sure nothing will be added if it does not
//         // have groups
//         if (has_groups) {
//             auto acq_mask = gr_pair.AcquisitionMask.get();
//             num_ch += has_groups*n_channels_acq(acq_mask);
//         } else {
//             num_ch = static_cast<uint32_t>(res->GroupConfigs.size());
//         }
//     }

//     header += SingleDimToBinaryHeader(
//         "en_chs", Types::UINT8, num_ch);

//     header += SingleDimToBinaryHeader(
//         "trg_mask", Types::UINT32, 1);

//     header += SingleDimToBinaryHeader(
//         "thresholds", Types::UINT16, num_ch);

//     header += SingleDimToBinaryHeader(
//         "dc_offsets", Types::UINT16, num_ch);

//     header += SingleDimToBinaryHeader(
//         "dc_corrections", Types::UINT8, num_ch);

//     header += SingleDimToBinaryHeader(
//         "dc_range", Types::SINGLE, num_ch);

//     header += SingleDimToBinaryHeader(
//         "time_stamp", Types::UINT32, 1);

//     header += SingleDimToBinaryHeader(
//         "trg_source", Types::UINT32, 1);

//     // This part is the header for the SiPM pulses
//     // The pulses are saved as raw counts, so uint16 is enough
//     const std::string c_type = "uint16";
//     // Name of this block
//     const std::string c_sipm_name = "sipm_traces";

//     // These are all the data line dimensions
//     // RecordLengthxNumChannels
//     std::string record_length_s = std::to_string(g_config.RecordLength);
//     std::string num_channels_s = std::to_string(num_ch);

//     std::string data_header = c_sipm_name + ";";  // name
//     data_header += c_type + ";";          // type
//     data_header += record_length_s + ",";       // dim 1
//     data_header += num_channels_s + ";";          // dim 2

//     // uint16_t is required as the format requires it to be 16 unsigned max
//     uint16_t s = static_cast<uint16_t>((header + data_header).length());
//     std::string data_header_size = NumToBinString(s);

//     // 0 means that it will be calculated by the number of lines
//     // int32 is required
//     int32_t j = 0x00000000;
//     std::string num_data_lines = NumToBinString(j);

//     // binary format goes like: endianess, header size, header string
//     // then number of lines (in this case 0)
//     return endianness_s + data_header_size
//         + header + data_header + num_data_lines;
// }

// // std::string sbc_init_file(CAENEvent& evt) noexcept {

// //  uint16_t numchannels = 0;
// //  uint16_t rl = 0;
// //  // We need  the channels that are enabled but for that it is actually
// //  // easier to count all the channels that have a size different than 0
// //  for(int i = 0; i < MAX_UINT16_CHANNEL_SIZE; i++){
// //    if(evt->Data->ChSize[i] > 0) {
// //      numchannels++;
// //      // all evt->Data->ChSize should be the same size...
// //      rl = evt->Data->ChSize[i];
// //    }
// //  }

// //  return sbc_init_file(rl, numchannels);
// // }

// std::string sbc_save_func(CAENEvent& evt, CAEN& res) noexcept {
//     // TODO(Hector): so this code could be improved and half of the reason
//     // it is related to the format itself. A bunch of the details
//     // it is saving with each line should not be saved every time. They are
//     // run constants, but that is how it is for now. Maybe in the future
//     // it will change.

//     // TODO(Zhiheng): add the code you need to save the channels on
//     // a group by using the mask.

//     // order of header:
//     // Name           type    length (B)    is Constant?
//     // sample_rate    double    8             Y
//     // en_chs         uint8     1*ch_size     Y
//     // trg_mask       uint32    4             Y
//     // thresholds     uint16    2*ch_size     Y
//     // dc_offsets     uint16    2*ch_size     Y
//     // dc_corrections uint8     1*ch_size     Y
//     // dc_range       single    4*ch_size     Y
//     // time_stamp     uint32    4             N
//     // trg_source     uint32    4             N
//     // data           uint16    2*rl*ch_size  N
//     //
//     // Total length         20 + ch_size(10 + 2*recordlength)

//     const auto rl = res->GlobalConfig.RecordLength;
//     const uint8_t ch_per_group = res->ModelConstants.NumChannelsPerGroup;
//     const uint8_t num_groups = res->ModelConstants.NumberOfGroups;
//     const bool has_groups = num_groups > 0;

//     static std::function<uint32_t(uint32_t)> n_channels_acq = [&](uint8_t acq_mask) {
//         return acq_mask == 0 ? 0: (acq_mask & 1) + n_channels_acq(acq_mask>>1);
//     };

//     uint64_t file_offset = 0;
//     auto append_cstr = [](auto num, uint64_t& offset, char* str) {
//         char* ptr = reinterpret_cast<char*>(&num);
//         for (size_t i = 0; i < sizeof(num) / sizeof(char); i++) {
//             str[offset + i] = ptr[i];
//         }
//         offset += sizeof(num) / sizeof(char);
//     };

//     // This will switch between Zhiheng code and mine if it has groups
//     auto wrap_if_group = [=, grp_conf = res->GroupConfigs]
//         (uint64_t& offset, char* str, auto f) {
//         for (std::size_t gr_n = 0; gr_n < grp_conf.size(); gr_n++) {
//             auto gr_pair = grp_conf[gr_n];
//             if (has_groups) {
//                 for (int ch = 0; ch < ch_per_group; ch++) {
//                     auto acq_mask = gr_pair.AcquisitionMask.get();
//                     if (acq_mask & (1 << ch)) {
//                         f(gr_n, gr_pair, offset, str, ch);
//                     }
//                 }
//             } else {
//                 f(gr_n, gr_pair, offset, str, 0);
//             }
//         }
//     };

//     uint32_t num_ch = 0;
//     for (auto gr_pair : res->GroupConfigs) {
//         if (has_groups) {
//             auto acq_mask = gr_pair.AcquisitionMask.get();
//             num_ch += has_groups*n_channels_acq(acq_mask);
//         } else {
//             num_ch = res->GroupConfigs.size();
//         }
//     }

//     // No strings for this one as this is more efficient
//     const size_t kNumLines = 20 + (2*rl + 10)*num_ch;
//     char out_str[kNumLines];

//     // sample_rate
//     append_cstr(res->ModelConstants.AcquisitionRate, file_offset, &out_str[0]);

//     // en_chs
//     // for(auto gr_pair : res->GroupConfigs) {
//     //  for(int ch = 0; ch < ch_per_group; ch++) {
//     //    if (gr_pair.second.AcquisitionMask & (1<<ch)){
//     //      uint8_t channel = gr_pair.second.Number * ch_per_group + ch;
//     //      append_cstr(channel, offset, &out_str[0]);
//     //    }
//     //  }
//     // }

//     wrap_if_group(file_offset, &out_str[0],
//         [=](std::size_t grp_n, auto, uint64_t& offset, char* str, uint8_t ch) {
//             uint8_t channel = grp_n * ch_per_group + ch;
//             append_cstr(channel, offset, str);
//         });

//     // trg_mask
//     uint32_t trg_mask = 0;
//     for (std::size_t ch = 0; ch < res->GroupConfigs.size(); ch++) {
//         auto gr_pair = res->GroupConfigs[ch];
//         uint32_t pos = has_groups ? ch * ch_per_group
//         : ch;

//         auto trig_mask = gr_pair.TriggerMask.get();
//         trg_mask |= (trig_mask << pos);
//     }

//     append_cstr(trg_mask, file_offset, &out_str[0]);

//     // thresholds
//     // for(auto gr_pair : res->GroupConfigs) {
//     //  if(has_groups){
//     //    for(int ch = 0; ch < ch_per_group; ch++) {
//     //      if (gr_pair.second.AcquisitionMask & (1 << ch)){
//     //        append_cstr(gr_pair.second.TriggerThreshold, offset, &out_str[0]);
//     //      }
//     //    }
//     //  } else {
//     //    append_cstr(gr_pair.second.TriggerThreshold, offset, &out_str[0]);
//     //  }
//     // }

//     wrap_if_group(file_offset, &out_str[0],
//         [=](std::size_t, auto group, uint64_t& offset, char* str, uint8_t) {
//             append_cstr(group.TriggerThreshold, offset, str);
//         });

//     // dc_offsets
//     // for(auto gr_pair : res->GroupConfigs) {
//     //  for(int ch = 0; ch < ch_per_group; ch++) {
//     //    if (gr_pair.second.AcquisitionMask & (1<<ch)){
//     //      append_cstr(gr_pair.second.DCOffset, offset, &out_str[0]);
//     //    }
//     //  }
//     // }
//     wrap_if_group(file_offset, &out_str[0],
//         [=](std::size_t, auto group, uint64_t& offset, char* str, uint8_t) {
//             append_cstr(group.DCOffset, offset, str);
//         });

//     // dc_corrections
//     // for(auto gr_pair : res->GroupConfigs) {
//     //  for(int ch = 0; ch < ch_per_group; ch++) {
//     //    if (gr_pair.second.AcquisitionMask & (1<<ch)){
//     //      append_cstr(gr_pair.second.DCCorrections[ch], offset, &out_str[0]);
//     //    }
//     //  }
//     // }
//     wrap_if_group(file_offset, &out_str[0],
//         [=](std::size_t, auto group, uint64_t& offset, char* str, uint8_t ch) {
//             if (group.DCCorrections.size() == 8) {
//                 append_cstr(group.DCCorrections[ch], offset, str);
//             } else {
//                 uint8_t tmp = 0;
//                 append_cstr(tmp, offset, str);
//             }
//         });

//     // dc_range
//     // for(auto gr_pair : res->GroupConfigs) {
//     //  for(int ch = 0; ch < ch_per_group; ch++) {
//     //    if (gr_pair.second.AcquisitionMask & (1<<ch)){
//     //      float val = res->GetVoltageRange(gr_pair.second.Number);
//     //      append_cstr(val, offset, &out_str[0]);
//     //    }
//     //  }
//     // }

//     wrap_if_group(file_offset, &out_str[0],
//         [&](std::size_t gr_n, auto, uint64_t& off, char* str, uint8_t) {
//             float val = res->GetVoltageRange(gr_n);
//             append_cstr(val, off, str);
//         });

//     // time_stamp
//     append_cstr(evt->Info.TriggerTimeTag, file_offset, &out_str[0]);

//     // tgr_source
//     append_cstr(evt->Info.Pattern, file_offset, &out_str[0]);


//     // For CAEN data, each line is an Event which contains a 2-D array
//     // where the x-axis is the record length and the y-axis are the
//     // number of channels that are activated
//     auto& evtdata = evt->Data;
//     for (std::size_t gr = 0; gr < res->GroupConfigs.size(); gr++) {
//         auto gr_pair = res->GroupConfigs[gr];
//         if (has_groups){
//             for (int ch = 0; ch < ch_per_group; ch++) {
//                 auto acq_mask = gr_pair.AcquisitionMask.get();
//                 if (acq_mask & (1<<ch)) {
//                     for(uint32_t xp = 0; xp < evtdata->ChSize[gr*ch_per_group+ch]; xp++) {
//                         append_cstr(evtdata->DataChannel[ch][xp],
//                             file_offset, &out_str[0]);
//                     }
//                 }
//             }
//         } else {
//             if (evtdata->ChSize[gr] > 0) {
//                 for (uint32_t xp = 0; xp < evtdata->ChSize[gr]; xp++) {
//                     append_cstr(evtdata->DataChannel[gr][xp],
//                         file_offset, &out_str[0]);
//                 }
//             }
//         }
//     }

//     // However, we do convert to string at the end, I wonder if this
//     // is a big performance impact?
//     return std::string(out_str, kNumLines);
// }

}  // namespace SBCQueens

