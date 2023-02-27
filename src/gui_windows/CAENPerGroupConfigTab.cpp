#include "sbcqueens-gui/gui_windows/CAENPerGroupConfigTab.hpp"

// C STD includes
// C 3rd party includes
#include <imgui.h>

// C++ STD includes
#include <array>

// C++ 3rd party includes
// my includes
#include "sbcqueens-gui/caen_helper.hpp"

#include "sbcqueens-gui/gui_windows/ControlList.hpp"

namespace SBCQueens {

void CAENPerGroupConfigTab::init_tab(const toml::table& cfg) {
    auto int8_to_bool_array = [](const uint8_t& in) -> std::array<bool, 8> {
        std::array<bool, 8> out;
        for(std::size_t i = 0; i < 8; i++) {
            out[i] = in & (1 << i);
        }
        return out;
    };

    auto CAEN_conf = cfg["CAEN"];
    const uint8_t c_MAX_GROUPS = 8;
    for (uint8_t grp_n = 0; grp_n < c_MAX_GROUPS; grp_n++) {
        std::string ch_toml = "group" + std::to_string(grp_n);
        if (CAEN_conf[ch_toml].as_table()) {
            _sipm_data.GroupConfigs[grp_n] =
                CAENGroupConfig{
                    CAEN_conf[ch_toml]["Enabled"].value_or(false),  // Enabled?
                    int8_to_bool_array(
                        CAEN_conf[ch_toml]["TrgMask"].value_or<uint8_t>(0u)),
                    // TriggerMask uint8_t
                    int8_to_bool_array(
                        CAEN_conf[ch_toml]["AcqMask"].value_or<uint8_t>(0u)),
                    // AcquisitionMask uint8_t
                    CAEN_conf[ch_toml]["Offset"].value_or<uint16_t>(0x8000u),
                    // DCOffset uint8_t
                    {0, 0, 0 ,0 ,0, 0, 0, 0},  // DCCorrections
                    // uint8_t
                    CAEN_conf[ch_toml]["Range"].value_or<uint8_t>(0u),
                    // DCRange
                    CAEN_conf[ch_toml]["Threshold"].value_or<uint16_t>(0x8000u)
                    // TriggerThreshold

                };

            if (const toml::array* arr = CAEN_conf[ch_toml]["Corrections"].as_array()) {
                spdlog::info("Corrections exist");
                std::size_t j = 0;
                CAENGroupConfig& last_config = _sipm_data.GroupConfigs[grp_n];
                arr->for_each([&](auto&& elem) {
                    if(j > 7) {
                        return;
                    }

                    last_config.DCCorrections[j] = elem.value_or(0);
                    j++;
                });
            }
        }
    }
}

void CAENPerGroupConfigTab::draw() {
	static uint8_t channel_to_modify = 0;
    constexpr auto group_num_in  =
        get_control<ControlTypes::InputUINT8, "Group to modify">(SiPMGUIControls);
   	draw_control(group_num_in,
   		_sipm_data,
        channel_to_modify,
        ImGui::IsItemEdited,
        // Callback when IsItemEdited !
        [](SiPMAcquisitionData&) {
        	// Nothing
        }
    );

   	auto& channel = _sipm_data.GroupConfigs[channel_to_modify];
   	auto& channel_tgm = channel.TriggerMask;
   	auto& channel_acq = channel.AcquisitionMask;
    auto& channel_corrections = channel.DCCorrections;

    ImGui::PushItemWidth(100);
    constexpr auto group_en_cb = get_control<ControlTypes::Checkbox, "Group Enable">(SiPMGUIControls);
   	draw_control(group_en_cb,
   		_sipm_data,
        channel.Enabled,
        ImGui::IsItemEdited,
        // Callback when IsItemEdited !
        [&](SiPMAcquisitionData& twin) {
        	twin.GroupConfigs[channel_to_modify].Enabled = channel.Enabled;
        }
    );

    constexpr auto dc_offset  =
        get_control<ControlTypes::InputUINT32, "DC Offset">(SiPMGUIControls);
   	draw_control(dc_offset,
   		_sipm_data,
        channel.DCOffset,
        ImGui::IsItemEdited,
        // Callback when IsItemEdited !
        [&](SiPMAcquisitionData& twin) {
        	twin.GroupConfigs[channel_to_modify].DCOffset = channel.DCOffset;
        }
    );
    constexpr auto trigg_threshold  =
        get_control<ControlTypes::InputUINT32, "Threshold">(SiPMGUIControls);
   	draw_control(trigg_threshold,
   		_sipm_data,
        channel.TriggerThreshold,
        ImGui::IsItemEdited,
        // Callback when IsItemEdited !
        [&](SiPMAcquisitionData& twin) {
        	twin.GroupConfigs[channel_to_modify].TriggerThreshold
        		= channel.TriggerThreshold;
        }
    );
    ImGui::PopItemWidth();

    ImGui::Separator();
    ImGui::Text("Per Channel configuration:");
    ImGui::Separator();

    _expand_controls(_sipm_data,
                     channel_to_modify,
                     std::make_index_sequence<8>{});
}

} // namespace SBCQueens
