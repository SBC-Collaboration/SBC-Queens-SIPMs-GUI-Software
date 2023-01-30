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

using TextPosition_t = enum class TextPositionEnum { Top, Bottom, Left, Right };
enum class Type_t { InputText, Button, };
struct DrawingOptions {
    TextPosition_t TextPosition = TextPositionEnum::Left;
    Color_t TextColor = ImVec4{1.0, 1.0, 1.0, 1.0};
    Color_t ControlColor;
    Color_t ControlHoveredColor;
    Color_t ControlActiveColor;
    Size_t ControlSize = Size_t{0, 0};
};

struct ControlOptions {

};

// This is the interface class of all the controls
class Control {
 public:
    // DOE -> data of everything
    const std::string Label;
    const std::string Text;
    const std::string HelpText;
    const DrawingOptions DrawOptions;

    constexpr Control() = default;
    constexpr Control(const std::string& label, const std::string& text,
        const std::string& help_text,
        const DrawingOptions& draw_opts = DrawingOptions{}) :
        Label{label},
        Text{text},
        HelpText{help_text},
        DrawOptions{draw_opts}
    {}

    constexpr Control(const std::string& label, const std::string& text) :
        Control{label, text, ""}
    {}

    ~Control() = default;
};

template<typename DataType,
    typename OutType,
    typename ImGuiDrawFunc,
    typename Callback = std::function<void(DataType&)>,
    typename... Args>
bool draw(DataType& doe, const Control& control,
    OutType& out, ImGuiDrawFunc&& draw_func,
    std::function<bool(void)>&& condition, Callback&& callback, Args... args) {

    bool imgui_out_state = false;
    switch(control.DrawOptions.TextPosition) {
    case TextPositionEnum::Left:
        ImGui::Text("%s", control.Text.c_str()); ImGui::SameLine();
        imgui_out_state = draw_func(control, out, args...);
        break;
    case TextPositionEnum::Right:
        imgui_out_state = draw_func(control, out, args...); ImGui::SameLine();
        ImGui::Text("%s", control.Text.c_str());
        break;
    case TextPositionEnum::Bottom:
        imgui_out_state = draw_func(control, out, args...);
        ImGui::Text("%s", control.Text.c_str());
        break;
    case TextPositionEnum::Top:
    default:
        ImGui::Text("%s", control.Text.c_str());
        imgui_out_state = draw_func(control, out, args...);
        break;
    }

    if(condition()) {
        doe.Callback = callback;
        doe.Changed = true;
    }

    if(ImGui::IsItemHovered()) {
        ImGui::SetTooltip(control.HelpText.c_str());
    }

    return imgui_out_state;
}

bool InputText(const Control& control, std::string& out) {
    return ImGui::InputText(control.Label.c_str(), &out);
}

bool Button(const Control& control, bool& out) {
    out = ImGui::Button(control.Label.c_str(),
        control.DrawOptions.ControlSize);
    return out;
}


bool Checkbox(const Control& control, bool& out) {
    return ImGui::Checkbox(control.Label.c_str(), &out);
}

bool InputInt(const Control& control, int& out) {
    return ImGui::InputInt(control.Label.c_str(), &out);
}

bool InputFloat(const Control& control, float& out) {
    return ImGui::InputFloat(control.Label.c_str(), &out);
}

bool InputDouble(const Control& control, double& out) {
    return ImGui::InputDouble(control.Label.c_str(), &out);
}

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

    return ImGui::InputScalar(control.Label.c_str(), type, &out);
}

auto InputINT8 = InputScalar<int8_t>;
auto InputUINT8 = InputScalar<uint8_t>;
auto InputINT16 = InputScalar<int16_t>;
auto InputUINT16 = InputScalar<uint16_t>;
auto InputINT32 = InputScalar<int32_t>;
auto InputUINT32 = InputScalar<uint32_t>;
auto InputINT64 = InputScalar<int64_t>;
auto InputUINT64 = InputScalar<uint64_t>;

template<typename T> requires std::is_enum_v<T>
bool ComboBox(const Control& label, T& state,
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
    if (ImGui::BeginCombo(label.Label.c_str(), s_states[index].c_str())) {
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

template<class T, size_t N>
constexpr bool find_item(const std::array<T, N>& list, const std::string& name) {
    for (const auto& control : list) {
        if(name == control.Label) {
            return true;
        }
    }

    return false;
}

template<typename T, size_t N>
constexpr T get_item(const std::array<T, N>& list, const std::string& name) {
    static_assert(find_item(list, name), "Item is not in list");

    for (const auto& item : list) {
        if(name == item.Label) {
            return item;
        }
    }

    // Return the trivial version of this, but it should never execute
    // given the static_assert
    return T{};
}

// draw at the spot is placed and checks if the item exist in list
// Helps in debugging the code and compile time.
// Otherwise, use draw()
template<typename DataType,
    size_t N,
    typename OutType,
    typename ImGuiDrawFunc,
    typename Callback = std::function<void(DataType&)>,
    typename ...Args>
bool draw_at_spot(DataType& doe, const std::array<Control, N>& list,
    const std::string& name, OutType& out, ImGuiDrawFunc&& draw_func,
    std::function<bool(void)>&& condition, Callback&& callback,
    Args... args) {

    Control item = get_item(list, name);
    return draw(doe, item, out,
        std::forward<ImGuiDrawFunc>(draw_func),
        std::forward<std::function<bool(void)>>(condition),
        std::forward<Callback>(callback),
        std::forward<Args...>(args...));
}

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
