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
#include <functional>
#include <unordered_map>
#include <string>
#include <type_traits>
#include <vector>
#include <string_view>

// C++ 3rd party includes
#include <spdlog/spdlog.h> // for fmt::format
#include <misc/cpp/imgui_stdlib.h> // for use with std::string

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

constexpr static Color_t Red{1.0, 0.0, 0.0, 1.0};
constexpr static Color_t Green{0.0, 1.0, 0.0, 1.0};
constexpr static Color_t Blue{0.0, 0.0, 1.0, 1.0};
constexpr static Color_t White{1.0, 1.0, 1.0, 1.0};
constexpr static Color_t Black{0.0, 0.0, 0.0, 0.0};

using TextPosition_t = enum class TextPositionEnum {
    None, Top, Bottom, Left, Right
};
using NumericFormat_t = enum class NumericFormatEnum {
    Default, Scientific, HexFloat, Fixed
};
using PlotType_t = enum class PlotTypeEnum{ Line, Scatter };

struct DrawingOptions {
    // Location of the accompanying text. Most ImGUI controls already
    // have something similar, this is for those who do not (like button)
    TextPosition_t TextPosition = TextPositionEnum::None;
    // Color of the accompanying text.
    Color_t TextColor = White;
    // Color of the control in standby - only button supports it ATM.
    Color_t ControlColor = Blue;
    // Color of the control when hovered - only button supports it ATM.
    Color_t ControlHoveredColor = Blue;
    // Color of the control when active - only button supports it ATM.
    Color_t ControlActiveColor = Blue;
    // Size of the control - only button supports it ATM. 0s sizes
    // the button to the length of the text
    Size_t ControlSize = Size_t{0, 0};

    // Numeric indicator/controls specific
    double StepSize = 0.0;
    // Numeric indicator/controls specific - Follow printf conventions
    std::string_view Format = "%.3f";

    // Plot specific - What type of plot are we using
    PlotType_t PlotType = PlotTypeEnum::Line;
    // Plot specific - Clear on new data (maybe will  be removed later)
    bool CleanOnNewData = false;
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
    const DrawingOptions DrawOptions;

    constexpr ~Indicator() {}

    constexpr Indicator() = default;
    constexpr Indicator(
        const std::string_view& text, const std::string_view& help_text,
        const DrawingOptions& draw_opts = DrawingOptions{}) :
        Label{list.value},
        Text{text},
        HelpText{help_text},
        DrawOptions{draw_opts}
    {}

    explicit constexpr Indicator(const std::string_view& text) :
        Indicator{text, ""}
    {}
};

// Controls
template<StringLiteral Label>
bool InputText(const Control<ControlTypes::InputText, Label>&,
               std::string& out) {
    return ImGui::InputText(Label.value, &out);
}

template<StringLiteral Label>
bool Button(const Control<ControlTypes::Button, Label>& control,
            bool& out) {
    ImGui::PushStyleColor(ImGuiCol_Button,
        control.DrawOptions.ControlColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
        control.DrawOptions.ControlHoveredColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
        control.DrawOptions.ControlActiveColor);

    out = ImGui::Button(Label.value, control.DrawOptions.ControlSize);

    ImGui::PopStyleColor(3);
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
    return ImGui::InputInt(Label.value, &out, static_cast<int>(
        control.DrawOptions.StepSize));
}

template<StringLiteral Label>
bool InputFloat(const Control<ControlTypes::InputFloat, Label>& control,
                float& out)  {
    return ImGui::InputFloat(Label.value, &out,
        static_cast<float>(control.DrawOptions.StepSize),
        100.0f*static_cast<float>(control.DrawOptions.StepSize),
        std::string(control.DrawOptions.Format).c_str());
}

template<StringLiteral Label>
bool InputDouble(const Control<ControlTypes::InputDouble, Label>& control,
                 double& out) {
    return ImGui::InputDouble(Label.value, &out,
        control.DrawOptions.StepSize,
        100.0*control.DrawOptions.StepSize,
        std::string(control.DrawOptions.Format).c_str());
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

    const void* step = &control.DrawOptions.StepSize;
    // const void* big_step = &(100*control.DrawOptions.StepSize);
    return ImGui::InputScalar(Label.value, type, &out,
        step,
        step,
        std::string(control.DrawOptions.Format).c_str());
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

// Indicators
 // { Numerical, String, LED, Plot };
template<StringLiteral Label, typename InputType>
void Numerical(const Indicator<IndicatorTypes::Numerical, Label>& indicator,
    const InputType& in_value) {
    constexpr auto label_name = Label.value;
    constexpr auto format = indicator.DrawOptions.Format;
    ImGui::PushStyleColor(ImGuiCol_Button,
        indicator.DrawOptions.ControlColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
        indicator.DrawOptions.ControlHoveredColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
        indicator.DrawOptions.ControlActiveColor);

    if (not indicator.DrawOptions.Format.starts_with("%")) {
        throw "Not a valid format string. It must start with %";
    }

    auto format_string = "{:"
        + indicator.DrawOptions.Format.remove_prefix(1) + "}##{}";

    ImGui::Button(fmt::vformat(format_string, in_value, label_name).c_str(),
        indicator.DrawOptions.ControlSize);

    ImGui::PopStyleColor(3);
}

template<StringLiteral Label>
void String(const Indicator<IndicatorTypes::String, Label>& indicator,
    std::string_view in_value) {
    constexpr auto label_name = Label.value;
    ImGui::PushStyleColor(ImGuiCol_Button,
        indicator.DrawOptions.ControlColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
        indicator.DrawOptions.ControlHoveredColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
        indicator.DrawOptions.ControlActiveColor);

    ImGui::Button(fmt::format("{}##{}", in_value, label_name).c_str(),
        indicator.DrawOptions.ControlSize);

    ImGui::PopStyleColor(3);
}

// LED indicator in a boolean is passed down and the color of the indicator
// reflects if its on or off. OFF for now is always red. HSV = 0, 0.6, 0.6
template<StringLiteral Label>
void LED(const Indicator<IndicatorTypes::LED, Label>& indicator,
    const bool& in_value) {
    constexpr auto label_name = Label.value;

    if (in_value) {
        ImGui::PushStyleColor(ImGuiCol_Button,
            indicator.DrawOptions.ControlColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
            indicator.DrawOptions.ControlHoveredColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
            indicator.DrawOptions.ControlActiveColor);
    } else {
        const auto off_color =
            static_cast<ImVec4>(ImColor::HSV(0.0f, 0.6f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_Button, off_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, off_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, off_color);
    }

    ImGui::Button(fmt::format("##{}", label_name).c_str(),
        indicator.DrawOptions.ControlSize);

    ImGui::PopStyleColor(3);
}

// LED indicator in which an arbitrary value can be passed down and a pairing
// function to turn it into a boolean.
template<StringLiteral Label, typename InputType,
    typename ConditionFunc = std::function<bool(const InputType&)>>
void LED(const Indicator<IndicatorTypes::LED, Label>& indicator,
    const InputType& in_value, ConditionFunc&& condition) {
    LED<Label>(indicator, condition(in_value));
}






template<ControlTypes T, StringLiteral Label, typename... Types>
constexpr static auto get_control(const std::tuple<Types...>& list) {
    return std::get<Control<T, Label>>(list);
}

template<IndicatorTypes T, StringLiteral Label, typename... Types>
constexpr static auto get_indicator(const std::tuple<Types...>& list) {
    return std::get<Indicator<T, Label>>(list);
}

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

template<IndicatorTypes T, StringLiteral Label, typename InType, typename... Args>
constexpr void get_draw_function(const Indicator<T, Label>& indicator,
                                 InType& in, Args&&... args) {
    if constexpr (T == IndicatorTypes::Numerical) {
        Numerical(indicator, in);
    } else if constexpr (T == IndicatorTypes::String) {
        String(indicator, in);
    } else if constexpr (T == IndicatorTypes::LED) {
        static_assert(sizeof...(args) > 1 && not std::is_same_v<InType, bool>,
            "LED indicator does not take"
            "more than 1 optional argument if InType is not bool");

        // If called with 0 extra arguments, the in type should be bool
        static_assert(sizeof...(args) == 0 && std::is_same_v<InType, bool>,
            "LED in type should be bool if no transformation "
            "function is passed");

        if constexpr (sizeof...(args) == 0 && std::is_same_v<InType, bool>) {
            LED(indicator, in);
            return;
        } else if constexpr (sizeof...(args) == 1) {
            LED(indicator, in, args...);
            return;
        }
    } else if constexpr (T == IndicatorTypes::Plot) {
    }
}

template<typename DrawItem>
void __draw_imgui_begin(const DrawItem& item) {
    switch (item.DrawOptions.TextPosition) {
    case TextPositionEnum::Left:
        ImGui::Text("%s", std::string(item.Text).c_str()); ImGui::SameLine();
        break;
    case TextPositionEnum::Top:
        ImGui::Text("%s", std::string(item.Text).c_str());
        break;
    default:
        break;
    }
}

template<typename DrawItem>
void __draw_imgui_end(const DrawItem& item) {
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", std::string(item.HelpText).c_str());
    }

    switch (item.DrawOptions.TextPosition) {
    case TextPositionEnum::Right:
        ImGui::SameLine();
        ImGui::Text("%s", std::string(item.Text).c_str());
        break;
    case TextPositionEnum::Bottom:
        ImGui::Text("%s", std::string(item.Text).c_str());
        break;
    default:
        break;
    }
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

    __draw_imgui_begin(control);
    imgui_out_state = get_draw_function<T>(control, out, args...);
    __draw_imgui_end(control);

    if (condition()) {
        doe.Callback = callback;
        doe.Changed = true;
    }

    return imgui_out_state;
}

template<IndicatorTypes T, StringLiteral Label,
    typename DataType,
    typename InType,
    typename... Args>
bool draw_indicator(const Indicator<T, Label>& indicator,
    InType& in, const Args&... args) {
    __draw_imgui_begin(indicator);
    get_draw_function(indicator, in, args...);
    __draw_imgui_end(indicator);
}


}  // namespace SBCQueens
#endif
