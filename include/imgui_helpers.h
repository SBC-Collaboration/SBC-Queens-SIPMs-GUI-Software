#pragma once

#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h> // for use with std::string
#include <implot.h>
#include <readerwriterqueue.h>
#include <spdlog/spdlog.h>

namespace SBCQueens {

	// The base control class
	class Control {
	public:

		Control() { }
		explicit Control(std::string&& label);

		// Moving
		Control(Control&&) = default;
		Control& operator=(Control&&) = default;

		// No copying
		Control(const Control&&) = delete;

	protected:
		std::string _label;
	}; 

	// Sets the ID of the Control.
// 	template<typename QueueFunc>
// 	class CountManager {
// 	public:
// 		using type = QueueFunc;

// 		CountManager() {}
// 		explicit CountManager(CountManager&&) = delete;
// 		CountManager& operator=(CountManager&&) = delete;

// 		void operator()(Control<QueueFunc>& control, const std::string& label) { 
// 			auto pair = Map.insert({label, control});
// 			if(pair.second) {
// 				spdlog::info("Created object with label {0}", label);
// 			} else {
// 				spdlog::warn("Control already exists with label {0}", label);
// 			}
// 		}

// private: 
// 		static std::unordered_map<std::string, Control<QueueFunc>&> Map;
// 	};

// 	template<typename QueueFunc>
// 	std::unordered_map<std::string, Control<QueueFunc>&> CountManager<QueueFunc>::Map;
	// end count


	class InputText : public Control {

		std::unique_ptr<std::string> _in;

public:

		InputText() { }
		InputText(std::string&& label, const std::string& init_val);

		// When event is true, callback is added to the queue
		// If the QueueFunc is not callable (ie does not exist
		// or not a function) it calls callback
		template<typename QueueFunc, typename Condition, typename Callback>
		bool operator()(QueueFunc f, Condition condition, Callback callback) const {

			// First call the ImGUI API
			bool u = ImGui::InputText(this->_label.c_str(), _in.get());
			// The if must come after the ImGUI API call if not, it will not work
			if(condition()) {
				// Checks if QueueFunc is callable with input Callback
				if constexpr ( std::is_invocable_v<QueueFunc, Callback> ) {
					f(callback);
				} else {
					callback();
				}
			}

			// Return the ImGUI bool
			return u;
		}

		template<typename Condition, typename Callback>
		bool operator()(Condition condition, Callback callback) const {

			// First call the ImGUI API
			bool u = ImGui::InputText(this->_label.c_str(), _in.get());
			// The if must come after the ImGUI API call if not, it will not work
			if(condition()) {
				callback();
			}

			// Return the ImGUI bool
			return u;
		}

		std::string Get() const {
			return *_in.get();
		}


	}; 

	InputText make_intput_text(std::string&& label, const std::string& init_val);
	// struct InputText

	class Button : public Control {

public:

		Button() {}
		explicit Button(std::string&& label);

		// When event is true, callback is added to the queue
		// If the QueueFunc is not callable (ie does not exist
		// or not a function) it calls callback
		template<typename QueueFunc, typename Callback>
		bool operator()(QueueFunc f, Callback callback) {

			bool state = ImGui::Button(this->_label.c_str());
			
			// The if must come after the ImGUI API call if not,
			// it will not work
			if(state) {
				// Checks if QueueFunc is callable with input Callback
				if constexpr ( std::is_invocable_v<QueueFunc, Callback> ) {
					f(callback);
				} else {
					callback();
				}
			}

			return state;
		}

		template<typename QueueFunc, typename Callback>
		bool operator()(Callback callback) {

			bool state = ImGui::Button(this->_label.c_str());
			
			// The if must come after the ImGUI API call if not,
			// it will not work
			if(state) {

				callback();
			
			}

			return state;
		}
	};

	Button make_button(std::string&& label);
	// struct InputText
	
	class Checkbox : public Control {

		std::unique_ptr<bool> _in;

public:

		Checkbox() {}
		Checkbox(std::string&& label, const bool& init_val);

		// When event is true, callback is added to the queue
		// If the QueueFunc is not callable (ie does not exist
		// or not a function) it calls callback
		template<typename QueueFunc, typename Event, typename Callback>
		bool operator()(QueueFunc f, Event event, Callback callback) const {

			bool u = ImGui::Checkbox(this->_label.c_str(), _in.get());

			// The if must come after the ImGUI API call if not,
			// it will not work
			if(event()) {
				// Checks if QueueFunc is callable with input Callback
				if constexpr ( std::is_invocable_v<QueueFunc, Callback> ) {
					f(callback);
				} else {
					callback();
				}
			}

			return u;

		}

		template<typename Event, typename Callback>
		bool operator()(Event event, Callback callback) const {

			bool u = ImGui::Checkbox(this->_label.c_str(), _in.get());

			// The if must come after the ImGUI API call if not,
			// it will not work
			if(event()) {

				callback();

			}

			return u;

		}

		bool Get() const {
			return *_in.get();
		}
	}; 

	Checkbox make_checkbox(std::string&& label, const bool& init_val);
	// struct Checkbox

	class InputFloat : public Control {

		std::unique_ptr<float> _in;

public:

		InputFloat() {}
		InputFloat(std::string&& label, const float& init_val);

		// When event is true, callback is added to the queue
		// If the QueueFunc is not callable (ie does not exist
		// or not a function) it calls callback
		template<typename QueueFunc, typename Event, typename Callback>
		bool operator()( QueueFunc f,
			const float& step, const float& step_fast,
			const std::string& format, Event event, Callback callback) const {

			bool u = ImGui::InputFloat(this->_label.c_str(),
				_in.get(), step, step_fast, format.c_str());

			// The if must come after the ImGUI API call if not, it will not work
			if(event()) {

				f(callback);

			}

			return u;
		}

		template<typename Event, typename Callback>
		bool operator()(
			const float& step, const float& step_fast,
			const std::string& format, Event event, Callback callback) const {

			bool u = ImGui::InputFloat(this->_label.c_str(),
				_in.get(), step, step_fast, format.c_str());

			// The if must come after the ImGUI API call if not, it will not work
			if(event()) {

				callback();

			}

			return u;
		}

		float Get() const {
			return *_in.get();
		}

	};

	InputFloat make_input_float(std::string&& label, const float& init_val);


} // namespace SBCQueens