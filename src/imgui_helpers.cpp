#include "imgui_helpers.h"


namespace SBCQueens {


	Control::Control(std::string&& label) : _label(label) {

	}

	InputText::InputText(std::string&& label, const std::string& init_val) 
			: Control(std::forward<std::string>(label)),
			_in(std::make_unique<std::string>(init_val)) { 

	}

	InputText make_intput_text(std::string&& label, const std::string& init_val){
		return InputText(std::forward<std::string>(label), init_val);
	}

	// struct InputText

	Button::Button(std::string&& label) 
			: Control(std::forward<std::string>(label)) {
	}

	Button make_button(std::string&& label){
		return Button(std::forward<std::string>(label));
	}
	// struct Button

	Checkbox::Checkbox(std::string&& label, const bool& init_val) 
		: Control(std::forward<std::string>(label)),
		_in(std::make_unique<bool>(init_val)) {
	}

	Checkbox make_checkbox(std::string&& label, const bool& init_val) {
		return Checkbox(std::forward<std::string>(label), init_val);
	}
	// struct Checkbox

	InputFloat::InputFloat(std::string&& label, const float& init_val) 
		: Control(std::forward<std::string>(label)),
		_in(std::make_unique<float>(init_val)) {
	}

	InputFloat make_input_float(std::string&& label, const float& init_val) {
		return InputFloat(std::forward<std::string>(label), init_val);
	}
	// struct InputFloat

} // namespace SBCQueens