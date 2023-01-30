#ifndef OTHERSMALLTABS_H
#define OTHERSMALLTABS_H
#include <toml.hpp>
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
// C++ 3rd party includes
#include <imgui.h>
#include <implot.h>

// my includes
#include "sbcqueens-gui/gui_windows/Window.hpp"

namespace SBCQueens {

template <typename Pipes>
class GuiConfigTab : public Tab<Pipes> {
 public:

 	explicit GuiConfigTab(const Pipes& p) : Tab<Pipes>(p, "GUI") {}
 	~GuiConfigTab() { }

 	void init_tab(const toml::table&) {}

 	void draw() {
        ImGui::SliderFloat("Plot line-width",
            &ImPlot::GetStyle().LineWeight, 0.0f, 10.0f);
        ImGui::SliderFloat("Plot Default Size Y",
            &ImPlot::GetStyle().PlotDefaultSize.y, 0.0f, 1000.0f);

        ImGui::EndTabItem();
 	}
};

template<typename Pipes>
GuiConfigTab<Pipes> make_gui_config_tab(const Pipes& p) {
    return GuiConfigTab<Pipes>(p);
}

}  // namespace SBCQueens


#endif