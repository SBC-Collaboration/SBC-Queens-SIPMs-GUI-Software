#ifndef IMGUIHELPERS_H
#define IMGUIHELPERS_H
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
struct Control {
    const ControlTypes ControlType = ControlTypes::InputText;
    const std::string_view Label = "";
    const std::string_view Text = "";
    const std::string_view HelpText = "";
    const DrawingOptions DrawOptions = DrawingOptions{};

    constexpr Control() = default;
    constexpr Control(const ControlTypes& ct, const std::string_view& label,
        const std::string_view& text, const std::string_view& help_text,
        const DrawingOptions& draw_opts = DrawingOptions{}) :
        ControlType{ct},
        Label{label},
        Text{text},
        HelpText{help_text},
        DrawOptions{draw_opts}
    {}

    constexpr Control(const ControlTypes& ct, const std::string_view& label,
        const std::string_view& text) :
        Control{ct, label, text, ""}
    {}

    ~Control() = default;
};

// This could be useful if the way the control is drawn changes but for now
// it can be left alone.
// struct Painter {
//     const Control& control;

//     explicit Painter(const Control& c) : control{c} {}

//     template <typename T>
//     bool draw(T& out) {

//     }
// };

bool InputText(const Control& control, std::string& out);
bool Button(const Control& control, bool& out);
bool Checkbox(const Control& control, bool& out);
bool InputInt(const Control& control, int& out);
bool InputFloat(const Control& control, float& out);
bool InputDouble(const Control& control, double& out);

template<typename T> requires std::is_integral_v<T>
bool InputScalar(const Control& control, T& out) {
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

bool InputINT8(const Control& control, int8_t& out);
bool InputUINT8(const Control& control, uint8_t& out);
bool InputINT16(const Control& control, int16_t& out);
bool InputUINT16(const Control& control, uint16_t& out);
bool InputINT32(const Control& control, int32_t& out);
bool InputUINT32(const Control& control, uint32_t& out);
bool InputINT64(const Control& control, int64_t& out);
bool InputUINT64(const Control& control, uint64_t& out);

template<typename T> requires std::is_enum_v<T>
bool ComboBox(const Control& control, T& state,
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

// Queue is the concurrent queue used to communicate between multi-threaded
// programs, and QueueType is the function type that sends information around
// template<typename Queue>
// class ControlLink {
//  private:
//     Queue _q;

//  public:
//     using QueueType = typename Queue::value_type;
//     using QueueFunc = std::function<bool(QueueType&&)>;

//     // f -> reference to the concurrent queue
//     // node -> toml node associated with this factory
//     explicit ControlLink(Queue q)
//         : _q(q) { }

//     // No moving nor copying
//     ControlLink(ControlLink&&) = delete;
//     ControlLink(const ControlLink&) = delete;

//     // When event is true, callback is added to the queue
//     // If the Callback cannot be added to the queue, then it calls callback
//     template<typename Condition, typename Callback>
//     bool InputText(const std::string& label, std::string& value,
//         Condition&& condition, Callback&& callback) {
//         // First call the ImGUI API
//         bool u = ImGui::InputText(label.c_str(), &value);
//         // The if must come after the ImGUI API call if not, it will not work
//         if (condition()) {
//             // Checks if QueueFunc is callable with input Callback
//             if constexpr ( std::is_invocable_v<QueueFunc, Callback> ) {
//                 _q->try_enqueue(std::forward<Callback>(callback));
//             } else {
//                 callback();
//             }
//         }

//         // Return the ImGUI bool
//         return u;
//     }

//     template<typename Callback>
//     bool Button(const std::string& label, Callback&& callback) {
//         bool state = ImGui::Button(label.c_str());

//         // The if must come after the ImGUI API call if not,
//         // it will not work
//         if (state) {
//             // Checks if QueueFunc is callable with input Callback
//             if constexpr ( std::is_invocable_v<QueueFunc, Callback> ) {
//                 _q->try_enqueue(std::forward<Callback>(callback));
//             } else {
//                 callback();
//             }
//         }

//         return state;
//     }

//     template<typename Condition, typename Callback>
//     bool Checkbox(const std::string& label, bool& value,
//         Condition&& condition, Callback&& callback) {
//         bool u = ImGui::Checkbox(label.c_str(), &value);

//         // The if must come after the ImGUI API call if not,
//         // it will not work
//         if (condition()) {
//             // Checks if QueueFunc is callable with input Callback
//             if constexpr ( std::is_invocable_v<QueueFunc, Callback> ) {
//                 _q.try_enqueue(std::forward<Callback>(callback));
//             } else {
//                 callback();
//             }
//         }

//         return u;
//     }

//     template<typename Condition, typename Callback>
//     bool InputFloat(const std::string& label, float& value,
//         const float& step, const float& step_fast,
//         const std::string& format,
//         Condition&& condition, Callback&& callback) {
//         bool u = ImGui::InputFloat(label.c_str(),
//             &value, step, step_fast, format.c_str());

//         // The if must come after the ImGUI API call if not, it will not work
//         if (condition()) {
//             if constexpr ( std::is_invocable_v<QueueFunc, Callback> ) {
//                 _q.try_enqueue(std::forward<Callback>(callback));
//             } else {
//                 callback();
//             }
//         }

//         return u;
//     }

//     template<typename T, typename Condition, typename Callback>
//     bool InputScalar(const std::string& label, T& value,
//         Condition&& condition, Callback&& callback) {
//         ImGuiDataType_ type = ImGuiDataType_S8;
//         if constexpr ( std::is_same_v<T, int8_t>) {
//             type = ImGuiDataType_S8;
//         } else if constexpr ( std::is_same_v<T, uint8_t>) {
//             type = ImGuiDataType_U8;
//         } else if constexpr ( std::is_same_v<T, int16_t>) {
//             type = ImGuiDataType_S16;
//         } else if constexpr ( std::is_same_v<T, uint16_t>) {
//             type = ImGuiDataType_U16;
//         } else if constexpr ( std::is_same_v<T, int32_t>) {
//             type = ImGuiDataType_S32;
//         } else if constexpr ( std::is_same_v<T, uint32_t>) {
//             type = ImGuiDataType_U32;
//         } else if constexpr ( std::is_same_v<T, int64_t>) {
//             type = ImGuiDataType_S64;
//         } else if constexpr ( std::is_same_v<T, uint64_t>) {
//             type = ImGuiDataType_U64;
//         } else {
//             assert(false);
//         }

//         bool u = ImGui::InputScalar(label.c_str(), type, &value);

//         // The if must come after the ImGUI API call if not, it will not work
//         if (condition()) {
//             if constexpr ( std::is_invocable_v<QueueFunc, Callback> ) {
//                 _q.try_enqueue(std::forward<Callback>(callback));
//             } else {
//                 callback();
//             }
//         }

//         return u;
//     }

//     template<typename T, typename Condition, typename Callback>
//     bool ComboBox(const std::string& label,
//         T& state,
//         const std::unordered_map<T, std::string>& map,
//         Condition&& condition, Callback&& callback) {
//         static size_t index = 0;
//         size_t i = 0;

//         std::vector<T> states;
//         std::vector<std::string> s_states;

//         for (auto pair : map) {
//             states.push_back(pair.first);
//             s_states.push_back(pair.second);

//             // This is to make sure the current selected item is the one
//             // that is already saved in state
//             if (state == pair.first) {
//                 index = i;
//             }

//             i++;
//         }

//         // bool u = ImGui::Combo(label.c_str(), &index, list.c_str());
//         if (ImGui::BeginCombo(label.c_str(), s_states[index].c_str())) {
//             for (i = 0; i < s_states.size(); i++) {
//                 const bool is_selected = (index == i);
//                 if (ImGui::Selectable(s_states[i].c_str(), is_selected)) {
//                     index = i;
//                 }
//                 // Set the initial focus when opening the combo
//                 // (scrolling + keyboard navigation focus)
//                 if (is_selected) {
//                     ImGui::SetItemDefaultFocus();
//                 }
//             }

//             ImGui::EndCombo();
//         }

//         state = states[index];

//         if (condition()) {
//             if constexpr ( std::is_invocable_v<QueueFunc, Callback> ) {
//                 _q.try_enqueue(std::forward<Callback>(callback));
//             } else {
//                 callback();
//             }
//         }

//         return true;
//     }

//     template<typename T, typename Condition, typename Callback>
//     bool ComboBox(const std::string& label,
//         T& state,
//         const std::unordered_map<std::string, T>& map,
//         Condition&& condition, Callback&& callback) {
//         static size_t index = 0;
//         size_t i = 0;

//         std::vector<T> states;
//         std::vector<std::string> s_states;
//         for (auto pair : map) {
//             states.push_back(pair.second);
//             s_states.push_back(pair.first);

//             if (state == pair.second) {
//                 index = i;
//             }

//             i++;
//         }

//         // bool u = ImGui::Combo(label.c_str(), &index, list.c_str());
//         if (ImGui::BeginCombo(label.c_str(), s_states[index].c_str())) {
//             for (i = 0; i < s_states.size(); i++) {
//                 const bool is_selected = (index == i);
//                 if (ImGui::Selectable(s_states[i].c_str(), is_selected)) {
//                     index = i;
//                 }
//                 // Set the initial focus when opening the combo
//                 // (scrolling + keyboard navigation focus)
//                 if (is_selected) {
//                     ImGui::SetItemDefaultFocus();
//                 }
//             }

//             ImGui::EndCombo();
//         }

//         state = states[index];

//         if (condition()) {
//             if constexpr ( std::is_invocable_v<QueueFunc, Callback> ) {
//                 _q.try_enqueue(std::forward<Callback>(callback));
//             } else {
//                 callback();
//             }
//         }

//         return true;
//     }
// };

template<size_t N>
constexpr static Control get_control(const std::array<Control, N>& list, std::string_view name) {
    auto out = std::find_if(std::cbegin(list), std::cend(list), [_name = name](auto& val) -> bool {
        return val.Label == _name;
    });

    // static_assert(out == std::cend(list), "Blah");
    return *out;
}

template<size_t N>
constexpr static ControlTypes get_control_type(const std::array<Control, N>& list, std::string_view name) {
    return get_control(list, name).ControlType;
}

template<ControlTypes T, typename OutType, typename... Args>
bool get_draw_function(const Control& control, OutType& out, Args&&... args) {
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

template<
    typename DataType,
    typename DrawFunc,
    typename OutType,
    typename Callback = std::function<void(DataType&)>,
    typename... Args>
bool draw_control(DataType& doe, const Control& control, DrawFunc&& f,
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

    imgui_out_state = f(control, out, args...);

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
