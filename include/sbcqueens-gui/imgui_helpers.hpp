#ifndef IMGUIHELPERS_H
#define IMGUIHELPERS_H
#include "sbcqueens-gui/implot_helpers.hpp"
#pragma once

// C STD includes
// C 3rd party includes
#include <imgui.h>
#include <implot.h>

// C++ STD includes
#include <ranges>
#include <span>
#include <variant>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <functional>
#include <any>

// C++ 3rd party includes
#include <misc/cpp/imgui_stdlib.h> // for use with std::string
#include <readerwriterqueue.h>
#include <spdlog/spdlog.h>
#include <toml.hpp>

namespace SBCQueens {

using Color_t = ImVec4;
using Size_t = ImVec2;

using TextPosition_t = enum class TextPositionEnum { None, Top, Bottom, Left, Right };
struct DrawingOptions {
    TextPosition_t TextPosition = TextPositionEnum::None;
    Color_t TextColor = ImVec4{1.0, 1.0, 1.0, 1.0};
    Color_t ControlColor;
    Color_t ControlHoveredColor;
    Color_t ControlActiveColor;
    Size_t ControlSize = Size_t{0, 0};
};

struct ControlOptions {

};

enum class ControlTypes { InputText, Button, Checkbox, InputInt, InputFloat,
    InputDouble, ComboBox, InputINT8, InputUINT8, InputINT16, InputUINT16, InputINT32,
    InputUINT32, InputINT64, InputUINT64 };

// This is the interface class of all the controls
struct GraphicalItemInterface {
    const std::string_view Label = "";
    const std::string_view Text = "";
    const std::string_view HelpText = "";
    const DrawingOptions DrawOptions = DrawingOptions{};

    constexpr ~GraphicalItemInterface() {}

protected:
    constexpr GraphicalItemInterface() = default;
    constexpr GraphicalItemInterface(const std::string_view& label,
        const std::string_view& text, const std::string_view& help_text,
        const DrawingOptions& draw_opts = DrawingOptions{}) :
        // ControlType{ct},
        Label{label},
        Text{text},
        HelpText{help_text},
        DrawOptions{draw_opts}
    {}

    constexpr GraphicalItemInterface(const std::string_view& label,
        const std::string_view& text) :
        GraphicalItemInterface{label, text, ""}
    {}
};

template<ControlTypes ControlType>
struct Control : public GraphicalItemInterface {
    constexpr Control() : GraphicalItemInterface{} {}
    constexpr Control(const std::string_view& label,
        const std::string_view& text, const std::string_view& help_text,
        const DrawingOptions& draw_opts = DrawingOptions{}) :
        GraphicalItemInterface{label, text, help_text, draw_opts}
    {}

    constexpr Control(const std::string_view& label,
        const std::string_view& text) :
        GraphicalItemInterface{label, text, ""}
    {}

    constexpr ~Control() {}
};

enum class IndicatorTypes { Numerical, String, LED };

// template<IndicatorTypes IndicatorType>
// struct Indicator_t : GraphicalItemInterface {
//     const std::string_view Label = "";
//     const std::string_view Text = "";
//     const std::string_view HelpText = "";
//     const DrawingOptions = DrawingOptions{};

//     constexpr Indicator() = default;
//     constexpr Indicator(const IndicatorTypes& ct, const std::string_view& label,
//         const std::string_view& text, const std::string_view& help_text,
//         const DrawingOptions& draw_opts = DrawingOptions{}) :
//         IndicatorType{ct},
//         Label{label},
//         Text{text},
//         HelpText{help_text},
//         DrawOptions{draw_opts}
//     {}

//     constexpr Indicator(const IndicatorTypes& ct, const std::string_view& label,
//         const std::string_view& text) :
//         Indicator{ct, label, text, ""}
//     {}

// };

// This could be useful if the way the control is drawn changes but for now
// it can be left alone.
// struct Painter {
//     const Control& control;

//     explicit Painter(const Control& c) : control{c} {}

//     template <typename T>
//     bool draw(T& out) {

//     }
// };

// Controls
bool InputText(const Control<ControlTypes::InputText>&, std::string&);
bool Button(const Control<ControlTypes::Button>&, bool&);
bool Checkbox(const Control<ControlTypes::Checkbox>&, bool&);
bool InputInt(const Control<ControlTypes::InputInt>&, int&);
bool InputFloat(const Control<ControlTypes::InputFloat>&, float&);
bool InputDouble(const Control<ControlTypes::InputDouble>&, double&);

template<ControlTypes ControlType, typename T> requires std::is_integral_v<T>
bool InputScalar(const Control<ControlType>& control, T& out) {
    ImGuiDataType_ type = ImGuiDataType_S8;
    if constexpr ( std::is_same_v<T, int8_t>) {
        type = ImGuiDataType_S8;
    } else if constexpr ( std::is_same_v<T, uint8_t>) {
        type = ImGuiDataType_U8;
    } else if constexpr ( std::is_same_v<T, int16_t>) {
        type = ImGuiDataType_S16;
    } else if constexpr ( std::is_same_v<T, uint16_t>) {
        type = ImGuiDataType_U16;
    } else if constexpr ( std::is_same_v<T, int32_t>) {
        type = ImGuiDataType_S32;
    } else if constexpr ( std::is_same_v<T, uint32_t>) {
        type = ImGuiDataType_U32;
    } else if constexpr ( std::is_same_v<T, int64_t>) {
        type = ImGuiDataType_S64;
    } else if constexpr ( std::is_same_v<T, uint64_t>) {
        type = ImGuiDataType_U64;
    }

    return ImGui::InputScalar(std::string(control.Label).c_str(), type, &out);
}

bool InputINT8(const Control<ControlTypes::InputINT8>&, int8_t&);
bool InputUINT8(const Control<ControlTypes::InputUINT8>&, uint8_t&);
bool InputINT16(const Control<ControlTypes::InputINT16>&, int16_t&);
bool InputUINT16(const Control<ControlTypes::InputUINT16>& control, uint16_t&);
bool InputINT32(const Control<ControlTypes::InputINT32>&, int32_t&);
bool InputUINT32(const Control<ControlTypes::InputUINT32>& control, uint32_t&);
bool InputINT64(const Control<ControlTypes::InputINT64>& control, int64_t&);
bool InputUINT64(const Control<ControlTypes::InputUINT64>& control, uint64_t&);

template<typename T> requires std::is_enum_v<T>
bool ComboBox(const Control<ControlTypes::ComboBox>& control, T& state,
    const std::unordered_map<T, std::string>& map) {
    static size_t index = 0;
    size_t i = 0;

    std::vector<T> states;
    std::vector<std::string> s_states;

    for (auto pair : map) {
        states.push_back(pair.first);
        s_states.push_back(pair.second);

        // This is to make sure the current selected item is the one
        // that is already saved in state
        if (state == pair.first) {
            index = i;
        }

        i++;
    }

    // bool u = ImGui::Combo(label.c_str(), &index, list.c_str());
    if (ImGui::BeginCombo(std::string(control.Label).c_str(), s_states[index].c_str())) {
        for (i = 0; i < map.size(); i++) {
            const bool is_selected = (index == i);
            if (ImGui::Selectable(s_states[i].c_str(), is_selected)) {
                index = i;
            }
            // Set the initial focus when opening the combo
            // (scrolling + keyboard navigation focus)
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    state = states[index];

    return true;
}

template<size_t N>
constexpr static GraphicalItemInterface get_control(const std::array<GraphicalItemInterface, N>& list, std::string_view name) {
    auto out = std::find_if(std::cbegin(list), std::cend(list), [_name = name](auto& val) -> bool {
        return val.Label == _name;
    });

    // static_assert(out == std::cend(list), "Blah");
    return *out;
}

template<size_t N>
constexpr static ControlTypes get_control_type(const std::array<GraphicalItemInterface, N>& list, std::string_view name) {
    return get_control(list, name).ControlType;
}

template<ControlTypes T, typename OutType, typename... Args>
bool get_draw_function(const Control<T>& control, OutType& out, Args&&... args) {
    if constexpr (T == ControlTypes::Button) {
        return Button(control, out);
    } else if constexpr(T == ControlTypes::Checkbox) {
        return Checkbox(control, out);
    } else if constexpr(T  == ControlTypes::InputInt) {
        return InputInt(control, out);
    } else if constexpr(T  ==  ControlTypes::InputFloat) {
        return InputFloat(control, out);
    } else if constexpr(T  ==  ControlTypes::InputDouble) {
        return InputDouble(control, out);
    } else if constexpr(T  ==  ControlTypes::ComboBox) {
        return ComboBox(control, out, args...);
    } else if constexpr(T  ==  ControlTypes::InputINT8) {
        return InputINT8(control, out);
    } else if constexpr(T  ==  ControlTypes::InputUINT8) {
        return InputUINT8(control, out);
    } else if constexpr(T  ==  ControlTypes::InputINT16) {
        return InputINT16(control, out);
    } else if constexpr(T  ==  ControlTypes::InputUINT16) {
        return InputUINT16(control, out);
    } else if constexpr(T  ==  ControlTypes::InputINT32) {
        return InputINT32(control, out);
    } else if constexpr(T  ==  ControlTypes::InputUINT32) {
        return InputUINT32(control, out);
    } else if constexpr(T  ==  ControlTypes::InputINT64) {
        return InputINT64(control, out);
    } else if constexpr(T  ==  ControlTypes::InputUINT64) {
        return InputUINT64(control, out);
    } else if constexpr(T  ==  ControlTypes::InputText) {
        return InputText(control, out);
    }
}

template<ControlTypes T,
    typename DataType,
    typename OutType,
    typename Callback = std::function<void(DataType&)>,
    typename... Args>
bool draw_control(const Control<T>& control, DataType& doe,
    OutType& out, std::function<bool(void)>&& condition,
    Callback&& callback, const Args&... args) {

    bool imgui_out_state = false;
    switch (control.DrawOptions.TextPosition) {
    case TextPositionEnum::Left:
        ImGui::Text("%s", std::string(control.Text).c_str()); ImGui::SameLine();
        break;
    case TextPositionEnum::Top:
        ImGui::Text("%s", std::string(control.Text).c_str());
        break;
    default:
        break;
    }

    imgui_out_state = get_draw_function<T>(control, out, args...);

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", std::string(control.HelpText).c_str());
    }

    switch (control.DrawOptions.TextPosition) {
    case TextPositionEnum::Right:
        ImGui::SameLine();
        ImGui::Text("%s", std::string(control.Text).c_str());
        break;
    case TextPositionEnum::Bottom:
        ImGui::Text("%s", std::string(control.Text).c_str());
        break;
    default:
        break;
    }

    if (condition()) {
        doe.Callback = callback;
        doe.Changed = true;
    }

    return imgui_out_state;
}

// template<
//     typename DataType,
//     typename OutType,
//     typename Callback = std::function<void(DataType&)>,
//     typename ...Args>
// bool draw_from_list(DataType& doe, std::string_view label, OutType& out,
//     std::function<bool(void)>&& condition, Callback&& callback,
//     Args&&... args) {

//     return draw<get_control_type(, label), DataType, OutType, Callback, Args...>(
//         doe,
//         get_control(SiPMGUIControls(), label),
//         out,
//         std::forward<std::function<bool(void)>>(condition),
//         std::forward<Callback>(callback),
//         std::forward<Args>(args)...);
// }

template<typename T, class Queue>
bool send_if_changed(const T& new_value, const Queue& queue) {
    if (new_value.Changed) {
        if (queue->try_enqueue(new_value)) {
            new_value.reset();
            return true;
        }
    }

    return false;
}

}  // namespace SBCQueens
#endif
