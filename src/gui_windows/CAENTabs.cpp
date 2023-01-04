#include "sbcqueens-gui/gui_windows/CAENTabs.hpp"

namespace SBCQueens {

bool CAENTabs::operator()() {
    if (ImGui::BeginTabItem("CAEN Global")) {
        ImGui::PushItemWidth(120);

        CAENControlFac.ComboBox("Model", cgui_state.Model,
            CAENDigitizerModelsMap, [](){ return false; },
            [](){ return true; });

        ImGui::InputScalar("Max Events Per Read", ImGuiDataType_U32,
            &cgui_state.GlobalConfig.MaxEventsPerRead);

        ImGui::InputScalar("Record Length [counts]", ImGuiDataType_U32,
            &cgui_state.GlobalConfig.RecordLength);
        ImGui::InputScalar("Post-Trigger buffer %", ImGuiDataType_U32,
            &cgui_state.GlobalConfig.PostTriggerPorcentage);

        ImGui::Checkbox("Overlapping Rejection",
            &cgui_state.GlobalConfig.TriggerOverlappingEn);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("If checked, overlapping rejection "
                "is disabled which leads to a non-constant record length. "
                "HIGHLY UNSTABLE FEATURE, DO NOT ENABLE.");
        }

        ImGui::Checkbox("TRG-IN as Gate",
            &cgui_state.GlobalConfig.EXTasGate);

        const std::unordered_map<CAEN_DGTZ_TriggerMode_t, std::string> tgg_mode_map = {
            {CAEN_DGTZ_TriggerMode_t::CAEN_DGTZ_TRGMODE_DISABLED, "Disabled"},
            {CAEN_DGTZ_TriggerMode_t::CAEN_DGTZ_TRGMODE_ACQ_ONLY, "Acq Only"},
            {CAEN_DGTZ_TriggerMode_t::CAEN_DGTZ_TRGMODE_EXTOUT_ONLY, "Ext Only"},
            {CAEN_DGTZ_TriggerMode_t::CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT, "Both"}};

        CAENControlFac.ComboBox("External Trigger Mode",
            cgui_state.GlobalConfig.EXTTriggerMode,
            tgg_mode_map,
            []() {return false;}, [](){});

        CAENControlFac.ComboBox("Software Trigger Mode",
            cgui_state.GlobalConfig.SWTriggerMode,
            tgg_mode_map,
            []() {return false;}, [](){});

        CAENControlFac.ComboBox("Trigger Polarity",
            cgui_state.GlobalConfig.TriggerPolarity,
            {{CAEN_DGTZ_TriggerPolarity_t::CAEN_DGTZ_TriggerOnFallingEdge, "Falling Edge"},
            {CAEN_DGTZ_TriggerPolarity_t::CAEN_DGTZ_TriggerOnRisingEdge, "Rising Edge"}},
            []() {return false;}, [](){});

        CAENControlFac.ComboBox("IO Level",
            cgui_state.GlobalConfig.IOLevel,
            {{CAEN_DGTZ_IOLevel_t::CAEN_DGTZ_IOLevel_NIM, "NIM"},
            {CAEN_DGTZ_IOLevel_t::CAEN_DGTZ_IOLevel_TTL, "TTL"}},
            []() {return false;}, [](){});

        CAENControlFac.Button("Software Trigger",
            [](CAENInterfaceData& state) {
                state.SoftwareTrigger = true;
                // software_trigger(state.);
                return true;
            });

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Forces a trigger in the digitizer if "
                "the feature is enabled");
        }

        ImGui::PopItemWidth();
        ImGui::EndTabItem();
    }

    // Channel tabs
    for (auto& ch : cgui_state.GroupConfigs) {
        std::string tab_name = "CAEN GR" + std::to_string(ch.Number);

        if (ImGui::BeginTabItem(tab_name.c_str())) {
            ImGui::PushItemWidth(120);
            // ImGui::Checkbox("Enabled", ch.);
            ImGui::InputScalar("DC Offset [counts]", ImGuiDataType_U16,
                &ch.DCOffset);

            ImGui::InputScalar("Threshold [counts]", ImGuiDataType_U16,
                &ch.TriggerThreshold);

            ImGui::InputScalar("Trigger Mask", ImGuiDataType_U8,
                &ch.TriggerMask);
            ImGui::InputScalar("Acquisition Mask", ImGuiDataType_U8,
                &ch.AcquisitionMask);

            int corr_i = 0;
            for (auto& corr : ch.DCCorrections) {
                std::string c_name = "Correction " + std::to_string(corr_i);
                ImGui::InputScalar(c_name.c_str(), ImGuiDataType_U8, &corr);
                corr_i++;
            }

            // TODO(Hector): change this to get the actual
            // ranges of the digitizer used, maybe change it to a dropbox?
            CAENControlFac.ComboBox("Range", ch.DCRange,
                {{0, "2V"}, {1, "0.5V"}},
                []() {return false;}, [](){});
            // ImGui::RadioButton("2V",
            // reinterpret_cast<int*>(&ch.DCRange), 0); ImGui::SameLine();
            // ImGui::RadioButton("0.5V",
            //  reinterpret_cast<int*>(&ch.DCRange), 1);

            ImGui::EndTabItem();
        }
    }
    return true;
}

}  // namespace SBCQueens
