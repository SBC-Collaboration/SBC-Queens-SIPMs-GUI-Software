#ifndef CAENTABS_H
#define CAENTABS_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
// C++ 3rd party includes
#include <imgui.h>

// my includes
#include "sbcqueens-gui/imgui_helpers.hpp"

#include "sbcqueens-gui/gui_windows/Window.hpp"
#include "sbcqueens-gui/gui_windows/ControlList.hpp"

#include "sbcqueens-gui/hardware_helpers/SiPMAcquisitionData.hpp"

// TODO(Hector): add CAEN per Channel config tab

namespace SBCQueens {

template<typename Pipes>
class CAENGeneralConfigTab : public Tab<Pipes> {
    using SiPMPipe_type = typename Pipes::SiPMPipe_type;
    SiPMAcquisitionPipeEnd<SiPMPipe_type> _sipm_pipe_end;
    SiPMAcquisitionData& _sipm_doe;

 public:
    explicit CAENGeneralConfigTab(const Pipes& p) :
        Tab<Pipes>{p, "CAEN Global"},
        _sipm_pipe_end{p.SiPMPipe}, _sipm_doe(_sipm_pipe_end.Doe)
    { }

    ~CAENGeneralConfigTab() {}

    void init_tab(const toml::table& tb) {
        auto CAEN_conf = tb["CAEN"];

        _sipm_doe.GlobalConfig.MaxEventsPerRead
            = CAEN_conf["MaxEventsPerRead"].value_or(512Lu);
        _sipm_doe.GlobalConfig.RecordLength
            = CAEN_conf["RecordLength"].value_or(2048Lu);
        _sipm_doe.GlobalConfig.PostTriggerPorcentage
            = CAEN_conf["PostBufferPorcentage"].value_or(50u);
        _sipm_doe.GlobalConfig.TriggerOverlappingEn
            = CAEN_conf["OverlappingRejection"].value_or(false);
        _sipm_doe.GlobalConfig.EXTasGate
            = CAEN_conf["TRGINasGate"].value_or(false);
        _sipm_doe.GlobalConfig.EXTTriggerMode
            = static_cast<CAEN_DGTZ_TriggerMode_t>(CAEN_conf["ExternalTrigger"].value_or(0L));
        _sipm_doe.GlobalConfig.SWTriggerMode
            = static_cast<CAEN_DGTZ_TriggerMode_t>(CAEN_conf["SoftwareTrigger"].value_or(0L));
        _sipm_doe.GlobalConfig.TriggerPolarity
            = static_cast<CAEN_DGTZ_TriggerPolarity_t>(CAEN_conf["Polarity"].value_or(0L));
        _sipm_doe.GlobalConfig.IOLevel
            = static_cast<CAEN_DGTZ_IOLevel_t>(CAEN_conf["IOLevel"].value_or(0));
    }

    void draw() {
        ImGui::PushItemWidth(120);

        draw_at_spot(_sipm_doe, SiPMControls, "Model",
            _sipm_doe.ConnectionType, ComboBox, ImGui::IsItemEdited,
            // Callback when IsItemEdited !
            [doe = _sipm_doe](SiPMAcquisitionData& caen_twin) {
                caen_twin.Model = doe.Model;
            },
            CAENDigitizerModelsMap);

        // CAENControlFac.ComboBox("Model", _sipm_doe.Model,
        //     CAENDigitizerModelsMap, [](){ return false; },
        //     [](){ return true; });

        ImGui::InputScalar("Max Events Per Read", ImGuiDataType_U32,
            &_sipm_doe.GlobalConfig.MaxEventsPerRead);

        ImGui::InputScalar("Record Length [counts]", ImGuiDataType_U32,
            &_sipm_doe.GlobalConfig.RecordLength);
        ImGui::InputScalar("Post-Trigger buffer %", ImGuiDataType_U32,
            &_sipm_doe.GlobalConfig.PostTriggerPorcentage);

        // ImGui::Checkbox("Overlapping Rejection",
        //     &_sipm_doe.GlobalConfig.TriggerOverlappingEn);
        // if (ImGui::IsItemHovered()) {
        //     ImGui::SetTooltip("If checked, overlapping rejection "
        //         "is disabled which leads to a non-constant record length. "
        //         "HIGHLY UNSTABLE FEATURE, DO NOT ENABLE.");
        // }

        ImGui::Checkbox("TRG-IN as Gate",
            &_sipm_doe.GlobalConfig.EXTasGate);

        const std::unordered_map<CAEN_DGTZ_TriggerMode_t, std::string> tgg_mode_map = {
            {CAEN_DGTZ_TriggerMode_t::CAEN_DGTZ_TRGMODE_DISABLED, "Disabled"},
            {CAEN_DGTZ_TriggerMode_t::CAEN_DGTZ_TRGMODE_ACQ_ONLY, "Acq Only"},
            {CAEN_DGTZ_TriggerMode_t::CAEN_DGTZ_TRGMODE_EXTOUT_ONLY, "Ext Only"},
            {CAEN_DGTZ_TriggerMode_t::CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT, "Both"}};

        draw_at_spot(_sipm_doe, SiPMControls, "External Trigger Mode",
            _sipm_doe.ConnectionType, ComboBox, ImGui::IsItemEdited,
            // Callback when IsItemEdited !
            [doe = _sipm_doe](SiPMAcquisitionData& caen_twin) {
                caen_twin.GlobalConfig.EXTTriggerMode = doe.GlobalConfig.EXTTriggerMode;
            },
            tgg_mode_map);
        // CAENControlFac.ComboBox("External Trigger Mode",
        //     _sipm_doe.GlobalConfig.EXTTriggerMode,
        //     tgg_mode_map,
        //     []() {return false;}, [](){});
        draw_at_spot(_sipm_doe, SiPMControls, "Software Trigger Mode",
            _sipm_doe.ConnectionType, ComboBox, ImGui::IsItemEdited,
            // Callback when IsItemEdited !
            [doe = _sipm_doe](SiPMAcquisitionData& caen_twin) {
                caen_twin.GlobalConfig.SWTriggerMode = doe.GlobalConfig.SWTriggerMode;
            },
            tgg_mode_map);
        // CAENControlFac.ComboBox("Software Trigger Mode",
        //     _sipm_doe.GlobalConfig.SWTriggerMode,
        //     tgg_mode_map,
        //     []() {return false;}, [](){});

        draw_at_spot(_sipm_doe, SiPMControls, "Trigger Polarity",
            _sipm_doe.ConnectionType, ComboBox, ImGui::IsItemEdited,
            // Callback when IsItemEdited !
            [doe = _sipm_doe](SiPMAcquisitionData& caen_twin) {
                caen_twin.GlobalConfig.TriggerPolarity = doe.GlobalConfig.TriggerPolarity;
            },
            {{CAEN_DGTZ_TriggerPolarity_t::CAEN_DGTZ_TriggerOnFallingEdge, "Falling Edge"},
            {CAEN_DGTZ_TriggerPolarity_t::CAEN_DGTZ_TriggerOnRisingEdge, "Rising Edge"}});
        // CAENControlFac.ComboBox("Trigger Polarity",
        //     _sipm_doe.GlobalConfig.TriggerPolarity,
        //     {{CAEN_DGTZ_TriggerPolarity_t::CAEN_DGTZ_TriggerOnFallingEdge, "Falling Edge"},
        //     {CAEN_DGTZ_TriggerPolarity_t::CAEN_DGTZ_TriggerOnRisingEdge, "Rising Edge"}},
        //     []() {return false;}, [](){});

        draw_at_spot(_sipm_doe, SiPMControls, "IO Level",
            _sipm_doe.ConnectionType, ComboBox, ImGui::IsItemEdited,
            // Callback when IsItemEdited !
            [doe = _sipm_doe](SiPMAcquisitionData& caen_twin) {
                caen_twin.GlobalConfig.IOLevel = doe.GlobalConfig.IOLevel;
            },
            {{CAEN_DGTZ_IOLevel_t::CAEN_DGTZ_IOLevel_NIM, "NIM"},
            {CAEN_DGTZ_IOLevel_t::CAEN_DGTZ_IOLevel_TTL, "TTL"}});
        // CAENControlFac.ComboBox("IO Level",
        //     _sipm_doe.GlobalConfig.IOLevel,
        //     {{CAEN_DGTZ_IOLevel_t::CAEN_DGTZ_IOLevel_NIM, "NIM"},
        //     {CAEN_DGTZ_IOLevel_t::CAEN_DGTZ_IOLevel_TTL, "TTL"}},
        //     []() {return false;}, [](){});

        bool tmp;
        draw_at_spot(_sipm_doe, SiPMControls, "Connect##CAEN",
            tmp, Button, [&](){ return tmp; },
            // Callback when tmp is true !
            [doe = _sipm_doe, run_dir = i_run_dir]
            (SiPMAcquisitionData& caen_twin) {
                caen_twin.SoftwareTrigger = true;
        });
        // CAENControlFac.Button("Software Trigger",
        //     [](CAENInterfaceData& state) {
        //         state.SoftwareTrigger = true;
        //         // software_trigger(state.);
        //         return true;
        //     });

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Forces a trigger in the digitizer if "
                "the feature is enabled");
        }

        ImGui::PopItemWidth();
        ImGui::EndTabItem();
    }

    // TODO(Hector): create this tab
    // Channel tabs
    // for (auto& ch : _sipm_doe.GroupConfigs) {
    //     std::string tab_name = "CAEN GR" + std::to_string(ch.Number);

    //     if (ImGui::BeginTabItem(tab_name.c_str())) {
    //         ImGui::PushItemWidth(120);
    //         // ImGui::Checkbox("Enabled", ch.);
    //         ImGui::InputScalar("DC Offset [counts]", ImGuiDataType_U16,
    //             &ch.DCOffset);

    //         ImGui::InputScalar("Threshold [counts]", ImGuiDataType_U16,
    //             &ch.TriggerThreshold);

    //         ImGui::InputScalar("Trigger Mask", ImGuiDataType_U8,
    //             &ch.TriggerMask);
    //         ImGui::InputScalar("Acquisition Mask", ImGuiDataType_U8,
    //             &ch.AcquisitionMask);

    //         int corr_i = 0;
    //         for (auto& corr : ch.DCCorrections) {
    //             std::string c_name = "Correction " + std::to_string(corr_i);
    //             ImGui::InputScalar(c_name.c_str(), ImGuiDataType_U8, &corr);
    //             corr_i++;
    //         }

    //         // TODO(Hector): change this to get the actual
    //         // ranges of the digitizer used, maybe change it to a dropbox?
    //         CAENControlFac.ComboBox("Range", ch.DCRange,
    //             {{0, "2V"}, {1, "0.5V"}},
    //             []() {return false;}, [](){});
    //         // ImGui::RadioButton("2V",
    //         // reinterpret_cast<int*>(&ch.DCRange), 0); ImGui::SameLine();
    //         // ImGui::RadioButton("0.5V",
    //         //  reinterpret_cast<int*>(&ch.DCRange), 1);

    //     }
    // }
};

template<typename Pipes>
CAENGeneralConfigTab<Pipes> make_caen_general_config_tab(const Pipes& p) {
    return CAENGeneralConfigTab<Pipes>(p);
}

}  // namespace SBCQueens
#endif
