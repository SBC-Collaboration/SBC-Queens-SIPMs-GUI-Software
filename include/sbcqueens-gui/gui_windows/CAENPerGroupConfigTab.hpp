#ifndef CAENPERGROUPCONFIGTAB_H
#define CAENPERGROUPCONFIGTAB_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <memory>

// C++ 3rd party includes
// my includes
#include "sbcqueens-gui/gui_windows/Window.hpp"

#include "sbcqueens-gui/hardware_helpers/SiPMAcquisitionData.hpp"
#include "sbcqueens-gui/gui_windows/ControlList.hpp"

namespace SBCQueens {

class CAENPerGroupConfigTab final : public Tab<SiPMAcquisitionData> {
	SiPMAcquisitionData& _sipm_data;
 public:
	explicit CAENPerGroupConfigTab(SiPMAcquisitionData& sipm_data) :
		Tab<SiPMAcquisitionData>("CAEN Group Config"), _sipm_data{sipm_data}
	{}

 	~CAENPerGroupConfigTab() = default;

    void init_tab(const toml::table& cfg) final;
 	void draw() final;

private:
    template <unsigned N>
    struct String {
        char c[N];
    };

    template<std::size_t i>
    constexpr static String<2> _to_string() {
        String<2> out = {};
        if constexpr (i == 0) {
            out.c[0] = '0';
        } else if constexpr (i == 1) {
            out.c[0] = '1';
        } else if constexpr (i == 2) {
            out.c[0] = '2';
        } else if constexpr (i == 3) {
            out.c[0] = '3';
        } else if constexpr (i == 4) {
            out.c[0] = '4';
        } else if constexpr (i == 5) {
            out.c[0] = '5';
        } else if constexpr (i == 6) {
            out.c[0] = '6';
        } else if constexpr (i == 7) {
            out.c[0] = '7';
        } else {
            out.c[0] = '0';
        }

        out.c[1] = '\0';

        return out;
    }

    // https://stackoverflow.com/questions/28708497/constexpr-to-concatenate-two-or-more-char-strings
    template<unsigned ...Len>
    constexpr static auto cat(const char (&...strings)[Len]) {
        constexpr unsigned N = (... + Len) - sizeof...(Len);
        String<N + 1> result = {};
        result.c[N] = '\0';

        char* dst = result.c;
        for (const char* src : {strings...}) {
            for (; *src != '\0'; src++, dst++) {
                *dst = *src;
            }
        }
        return result;
    }

    template<std::size_t... I>
    constexpr static void _expand_controls(SiPMAcquisitionData& data,
                                  const uint8_t& ch,
                                  std::index_sequence<I...>) {
        (_per_channel_draw<I>(data, ch),...);
    }

    template<std::size_t i>
    constexpr static void _per_channel_draw(SiPMAcquisitionData& data, const uint8_t& ch) {
        constexpr auto trg = cat("TRG", _to_string<i>().c);
        constexpr auto acq = cat("ACQ", _to_string<i>().c);
        constexpr auto correction = cat("Correction ", _to_string<i>().c);

        auto& channel = data.GroupConfigs[ch];
        auto& channel_tgm = channel.TriggerMask;
        auto& channel_acq = channel.AcquisitionMask;
        auto& channel_corrections = channel.DCCorrections;

        constexpr auto trg0  =
                get_control<ControlTypes::Checkbox, trg.c>(SiPMGUIControls);
        draw_control(trg0,
                     data,
                     channel_tgm[i],
                     ImGui::IsItemEdited,
                // Callback when IsItemEdited !
                     [&](SiPMAcquisitionData& twin) {
                         auto& t = twin.GroupConfigs[ch].TriggerMask[i];
                         t = channel_tgm[i];
                     }
        );

        ImGui::SameLine();

        constexpr auto acq0  =
                get_control<ControlTypes::Checkbox, acq.c>(SiPMGUIControls);
        draw_control(acq0,
                     data,
                     channel_acq[i],
                     ImGui::IsItemEdited,
                // Callback when IsItemEdited !
                     [&](SiPMAcquisitionData& twin) {
                         auto& t = twin.GroupConfigs[ch].AcquisitionMask[i];
                         t = channel_acq[i];
                     }
        );

        ImGui::SameLine();
        ImGui::PushItemWidth(100);
        constexpr auto correction1_ctr  =
                get_control<ControlTypes::InputUINT8, correction.c>(SiPMGUIControls);
        draw_control(correction1_ctr,
                     data,
                     channel_corrections[i],
                     ImGui::IsItemEdited,
                // Callback when IsItemEdited !
                     [&](SiPMAcquisitionData& twin) {
                         auto& t = twin.GroupConfigs[ch].DCCorrections[i];
                         t = channel_corrections[i];
                     }
        );
        ImGui::PopItemWidth();
    }
};

inline auto make_caen_group_config_tab(SiPMAcquisitionData& sipm_data) {
	return std::make_unique<CAENPerGroupConfigTab>(sipm_data);
}

} // namespace SBCQueens

#endif