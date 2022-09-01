#pragma once

#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <functional>
#include <any>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h> // for use with std::string
#include <implot.h>
#include <readerwriterqueue.h>
#include <spdlog/spdlog.h>
#include <toml.hpp>

namespace SBCQueens {

	// This is the interface class of all the controls
	template<typename QueueFunc>
	class Control {
public:
		virtual ~Control() {}

		// No moving
		Control(Control&&) = delete;

		// No copying
		Control(const Control&&) = delete;

		// Yes referencing
		Control(Control&) = default;
		Control& operator=(Control&) = default;

protected:

		Control() {}

		// Default constructor
		// label is ... the label of the control
		// text is name or text to show in ImGUI
		// q is the Queue send function
		Control(std::string label, std::string text, QueueFunc&& q)
			: _label(label), _imgui_text(text), _q(q) {}


		// toml ini file constructor
		// Allows for its initial value to be defined using the toml file
		// not to be used to define the GUI
		// label is ... the label of the control
		// text is name or text to show in ImGUI
		// q is the Queue send function
		Control(
			std::string label, std::string text, QueueFunc&& q,
			toml::node_view<toml::node> toml_node)
			: _label(label), _imgui_text(text), _q(q) {}

		std::string _label;
		std::string _imgui_text;

		QueueFunc _q;

	};

	// Queue is the concurrent queue used to communicate between multi-threaded
	// programs, and QueueType is the function type that sends information around
	template<typename Queue>
	class ControlLink {

private:
		Queue& _q;

public:
		using QueueType = typename Queue::value_type;
		using QueueFunc = std::function<bool(QueueType&&)>;

		// f -> reference to the concurrent queue
		// node -> toml node associated with this factory
		explicit ControlLink(Queue& q)
			: _q(q)
		{ }

		// No moving nor copying
		ControlLink(ControlLink&&) = delete;
		ControlLink(const ControlLink&) = delete;

		// When event is true, callback is added to the queue
		// If the Callback cannot be added to the queue, then it calls callback
		template<typename Condition, typename Callback>
		bool InputText(const std::string& label, std::string& value,
			Condition&& condition, Callback&& callback) {

			// First call the ImGUI API
			bool u = ImGui::InputText(label.c_str(), &value);
			// The if must come after the ImGUI API call if not, it will not work
			if(condition()) {
				// Checks if QueueFunc is callable with input Callback
				if constexpr ( std::is_invocable_v<QueueFunc, Callback> ) {
					_q.try_enqueue(std::forward<Callback>(callback));
				} else {
					callback();
				}
			}

			// Return the ImGUI bool
			return u;

		}

		template<typename Callback>
		bool Button(const std::string& label, Callback&& callback) {

			bool state = ImGui::Button(label.c_str());

			// The if must come after the ImGUI API call if not,
			// it will not work
			if(state) {
				// Checks if QueueFunc is callable with input Callback
				if constexpr ( std::is_invocable_v<QueueFunc, Callback> ) {
					_q.try_enqueue(std::forward<Callback>(callback));
				} else {
					callback();
				}
			}

			return state;

		}

		template<typename Condition, typename Callback>
		bool Checkbox(const std::string& label, bool& value,
			Condition&& condition, Callback&& callback) {

			bool u = ImGui::Checkbox(label.c_str(), &value);

			// The if must come after the ImGUI API call if not,
			// it will not work
			if(condition()) {
				// Checks if QueueFunc is callable with input Callback
				if constexpr ( std::is_invocable_v<QueueFunc, Callback> ) {
					_q.try_enqueue(std::forward<Callback>(callback));
				} else {
					callback();
				}
			}

			return u;

		}

		template<typename Condition, typename Callback>
		bool InputFloat(const std::string& label, float& value,
			const float& step, const float& step_fast,
			const std::string& format,
			Condition&& condition, Callback&& callback) {

			bool u = ImGui::InputFloat(label.c_str(),
				&value, step, step_fast, format.c_str());

			// The if must come after the ImGUI API call if not, it will not work
			if(condition()) {
				if constexpr ( std::is_invocable_v<QueueFunc, Callback> ) {
					_q.try_enqueue(std::forward<Callback>(callback));
				} else {
					callback();
				}

			}

			return u;

		}

		template<typename T, typename Condition, typename Callback>
		bool InputScalar(const std::string& label, T& value,
			Condition&& condition, Callback&& callback) {

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
			} else {
				assert(false);
			}

			bool u = ImGui::InputScalar(label.c_str(), type, &value);

			// The if must come after the ImGUI API call if not, it will not work
			if(condition()) {
				if constexpr ( std::is_invocable_v<QueueFunc, Callback> ) {
					_q.try_enqueue(std::forward<Callback>(callback));
				} else {
					callback();
				}

			}

			return u;
		}

		template<typename T, typename Condition, typename Callback>
		bool ComboBox(const std::string& label,
			T& state,
			const std::unordered_map<T, std::string>& map,
			Condition&& condition, Callback&& callback) {

			static size_t index = 0;
			size_t i = 0;

			std::string list = "";
			std::vector<T> states;
			std::vector<std::string> s_states;

			for(auto pair : map) {
				states.push_back(pair.first);
				s_states.push_back(pair.second);

				// This is to make sure the current selected item is the one
				// that is already saved in state
				if(state == pair.first) {
					index = i;
				}

				i++;
			}

			//bool u = ImGui::Combo(label.c_str(), &index, list.c_str());
		 	if (ImGui::BeginCombo(label.c_str(), s_states[index].c_str())) {

		 		for( i = 0; i < s_states.size(); i++ ) {

		 			const bool is_selected = (index == i);
					if (ImGui::Selectable(s_states[i].c_str(), is_selected)){
                    	index = i;
					}
                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                	if (is_selected) {
                    	ImGui::SetItemDefaultFocus();
                	}
		 		}

		 		ImGui::EndCombo();
			}

			state = states[index];

			if(condition()) {
				if constexpr ( std::is_invocable_v<QueueFunc, Callback> ) {
					_q.try_enqueue(std::forward<Callback>(callback));
				} else {
					callback();
				}
			}

			return true;

		}

		template<typename T, typename Condition, typename Callback>
		bool ComboBox(const std::string& label,
			T& state,
			const std::unordered_map<std::string, T>& map,
			Condition&& condition, Callback&& callback) {

			static size_t index = 0;
			size_t i = 0;

			std::string list = "";
			std::vector<T> states;
			std::vector<std::string> s_states;
			for(auto pair : map) {
				states.push_back(pair.second);
				s_states.push_back(pair.first);

				if(state == pair.second) {
					index = i;
				}

				i++;
			}

			//bool u = ImGui::Combo(label.c_str(), &index, list.c_str());
		 	if (ImGui::BeginCombo(label.c_str(), s_states[index].c_str())) {

		 		for( i = 0; i < s_states.size(); i++ ) {

		 			const bool is_selected = (index == i);
					if (ImGui::Selectable(s_states[i].c_str(), is_selected)){
                    	index = i;
					}
                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                	if (is_selected) {
                    	ImGui::SetItemDefaultFocus();
                	}
		 		}

		 		ImGui::EndCombo();
			}

			state = states[index];

			if(condition()) {
				if constexpr ( std::is_invocable_v<QueueFunc, Callback> ) {
					_q.try_enqueue(std::forward<Callback>(callback));
				} else {
					callback();
				}
			}

			return true;

		}


	};
} // namespace SBCQueens