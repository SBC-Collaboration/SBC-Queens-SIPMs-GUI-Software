#ifndef OTHERSMALLTABS_H
#define OTHERSMALLTABS_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
// C++ 3rd party includes
#include <toml.hpp>

// my includes
#include "sbcqueens-gui/gui_windows/Window.hpp"

namespace SBCQueens {

class GuiConfigTab : public Tab<> {
 public:
 	GuiConfigTab() : Tab<>("GUI") {}
 	~GuiConfigTab() { }

 private:
 	void init_tab(const toml::table&);
 	void draw();
};

inline auto make_gui_config_tab() {
    return std::make_unique<GuiConfigTab>();
}

}  // namespace SBCQueens


#endif