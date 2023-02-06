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
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <string_view>

// C++ 3rd party includes
#include <misc/cpp/imgui_stdlib.h> // for use with std::string
#include <readerwriterqueue.h>
#include <spdlog/spdlog.h>
#include <toml.hpp>

namespace SBCQueens {

// stolen from https://ctrpeach.io/posts/cpp20-string-literal-template-parameters/
// it is a great idea!
template<size_t N>
struct StringLiteral {
    constexpr StringLiteral(const char (&str)[N]) {
        // The explicit breaks this, maybe we can find a more suitable solution?
        std::copy_n(str, N, value);
    }

    char value[N];
};

using Color_t = ImVec4;
using Size_t = ImVec2;

using TextPosition_t = enum class TextPositionEnum {
    None, Top, Bottom, Left, Right
};

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
    InputDouble, ComboBox, InputINT8, InputUINT8, InputINT16, InputUINT16,
    InputINT32, InputUINT32, InputINT64, InputUINT64 };

template<ControlTypes ControlType, StringLiteral list>
struct Control {
    const std::string_view Label = "";
    const std::string_view Text = "";
    const std::string_view HelpText = "";
    const DrawingOptions DrawOptions = DrawingOptions{};

    constexpr ~Control() {}

    constexpr Control() = default;
    constexpr Control(
        const std::string_view& text, const std::string_view& help_text,
        const DrawingOptions& draw_opts = DrawingOptions{}) :
        Label{list.value},
        // ControlType{ct},
        Text{text},
        HelpText{help_text},
        DrawOptions{draw_opts}
    {}

    explicit constexpr Control(const std::string_view& text) :
        Control{text, ""}
    {}
};

enum class IndicatorTypes { Numerical, String, LED, Plot };

template<IndicatorTypes IndicatorType, StringLiteral list>
struct Indicator {
    const std::string_view Label = "";
    const std::string_view Text = "";
    const std::string_view HelpText = "";
    const DrawingOptions DrawOptions = DrawingOptions{};

    constexpr ~Indicator() {}

    constexpr Indicator() = default;
    constexpr Indicator(
        const std::string_view& text, const std::string_view& help_text,
        const DrawingOptions& draw_opts = DrawingOptions{}) :
        Label{list.value},
        // IndicatorType{ct},
        Text{text},
        HelpText{help_text},
        DrawOptions{draw_opts}
    {}

    explicit constexpr Indicator(const std::string_view& text) :
        Indicator{text, ""}
    {}
};

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
template<StringLiteral Label>
bool InputText(const Control<ControlTypes::InputText, Label>&,
               std::string& out) {
    return ImGui::InputText(Label.value, &out);
}

template<StringLiteral Label>
bool Button(const Control<ControlTypes::Button, Label>& control,
            bool& out) {
    out = ImGui::Button(Label.value, control.DrawOptions.ControlSize);
    return out;
}

template<StringLiteral Label>
bool Checkbox(const Control<ControlTypes::Checkbox, Label>& control,
              bool& out) {
    return ImGui::Checkbox(Label.value, &out);
}

template<StringLiteral Label>
bool InputInt(const Control<ControlTypes::InputInt, Label>& control,
              int& out) {
    return ImGui::InputInt(Label.value, &out);
}

template<StringLiteral Label>
bool InputFloat(const Control<ControlTypes::InputFloat, Label>& control,
                float& out)  {
    return ImGui::InputFloat(Label.value, &out);
}

template<StringLiteral Label>
bool InputDouble(const Control<ControlTypes::InputDouble, Label>& control,
                 double& out) {
    return ImGui::InputDouble(Label.value, &out);
}

template<ControlTypes ControlType, StringLiteral Label, typename T>
requires std::is_integral_v<T>
bool InputScalar(const Control<ControlType, Label>& control, T& out) {
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

    return ImGui::InputScalar(Label.value, type, &out);
}

template<StringLiteral Label>
bool InputINT8(const Control<ControlTypes::InputINT8, Label>& control,
               int8_t& out) {
    return InputScalar<ControlTypes::InputINT8, Label, int8_t>(control, out);
}

template<StringLiteral Label>
bool InputUINT8(const Control<ControlTypes::InputUINT8, Label>& control,
                uint8_t& out) {
    return InputScalar<ControlTypes::InputUINT8, Label, uint8_t>(control, out);
}

template<StringLiteral Label>
bool InputINT16(const Control<ControlTypes::InputINT16, Label>& control,
                int16_t& out) {
    return InputScalar<ControlTypes::InputINT16, Label, int16_t>(control, out);
}

template<StringLiteral Label>
bool InputUINT16(const Control<ControlTypes::InputUINT16, Label>& control,
                uint16_t& out) {
    return InputScalar<ControlTypes::InputUINT16, Label, uint16_t>(control, out);
}

template<StringLiteral Label>
bool InputINT32(const Control<ControlTypes::InputINT32, Label>& control,
                int32_t& out) {
    return InputScalar<ControlTypes::InputINT32, Label, int32_t>(control, out);
}

template<StringLiteral Label>
bool InputUINT32(const Control<ControlTypes::InputUINT32, Label>& control,
                uint32_t& out) {
    return InputScalar<ControlTypes::InputUINT32, Label, uint32_t>(control, out);
}

template<StringLiteral Label>
bool InputINT64(const Control<ControlTypes::InputINT64, Label>& control,
                int64_t& out) {
    return InputScalar<ControlTypes::InputINT64, Label, int64_t>(control, out);
}

template<StringLiteral Label>
bool InputUINT64(const Control<ControlTypes::InputUINT64, Label>& control,
                uint64_t& out) {
    return InputScalar<ControlTypes::InputUINT64, Label, uint64_t>(control, out);
}

template<StringLiteral Label, typename T> requires std::is_enum_v<T>
bool ComboBox(const Control<ControlTypes::ComboBox, Label>& control, T& state,
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
    if (ImGui::BeginCombo(Label, s_states[index].c_str())) {
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

template<ControlTypes T, StringLiteral Label, typename... Types>
constexpr static auto get_control(const std::tuple<Types...>& list) {
    return std::get<Control<T, Label>>(list);
}

// template<size_t N>
// constexpr static ControlTypes get_control_type(
//     const std::tuple<GraphicalItemInterface, N>& list, std::string_view name) {
//     return get_control(list, name).ControlType;
// }

template<ControlTypes T, StringLiteral Label, typename OutType, typename... Args>
constexpr bool get_draw_function(const Control<T, Label>& control,
                                 OutType& out, Args&&... args) {
    if constexpr (T == ControlTypes::Button) {
        return Button<Label>(control, out);
    } else if constexpr(T == ControlTypes::Checkbox) {
        return Checkbox<Label>(control, out);
    } else if constexpr(T  == ControlTypes::InputInt) {
        return InputInt<Label>(control, out);
    } else if constexpr(T  ==  ControlTypes::InputFloat) {
        return InputFloat<Label>(control, out);
    } else if constexpr(T  ==  ControlTypes::InputDouble) {
        return InputDouble<Label>(control, out);
    } else if constexpr(T  ==  ControlTypes::ComboBox) {
        return ComboBox<Label>(control, out, args...);
    } else if constexpr(T  ==  ControlTypes::InputINT8) {
        return InputINT8<Label>(control, out);
    } else if constexpr(T  ==  ControlTypes::InputUINT8) {
        return InputUINT8<Label>(control, out);
    } else if constexpr(T  ==  ControlTypes::InputINT16) {
        return InputINT16<Label>(control, out);
    } else if constexpr(T  ==  ControlTypes::InputUINT16) {
        return InputUINT16<Label>(control, out);
    } else if constexpr(T  ==  ControlTypes::InputINT32) {
        return InputINT32<Label>(control, out);
    } else if constexpr(T  ==  ControlTypes::InputUINT32) {
        return InputUINT32<Label>(control, out);
    } else if constexpr(T  ==  ControlTypes::InputINT64) {
        return InputINT64<Label>(control, out);
    } else if constexpr(T  ==  ControlTypes::InputUINT64) {
        return InputUINT64<Label>(control, out);
    } else if constexpr(T  ==  ControlTypes::InputText) {
        return InputText<Label>(control, out);
    }

    throw("Draw function not supported");
}

template<ControlTypes T, StringLiteral Label,
    typename DataType,
    typename OutType,
    typename Callback = std::function<void(DataType&)>,
    typename... Args>
bool draw_control(const Control<T, Label>& control,
    DataType& doe,
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
