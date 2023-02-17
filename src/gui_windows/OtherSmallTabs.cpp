#include "sbcqueens-gui/gui_windows/OtherSmallTabs.hpp"

// C STD includes
// C 3rd party includes
#include <imgui.h>
#include <implot.h>

// C++ STD includes
// C++ 3rd party includes
// my includes

namespace SBCQueens {

void GuiConfigTab::init_tab(const toml::table&) {}

void GuiConfigTab::draw() {
	ImGui::SliderFloat("Plot line-width",
	    &ImPlot::GetStyle().LineWeight, 0.0f, 10.0f);
	ImGui::SliderFloat("Plot Default Size X",
	    &ImPlot::GetStyle().PlotDefaultSize.x, 0.0f, 1000.0f);
	ImGui::SliderFloat("Plot Default Size Y",
	    &ImPlot::GetStyle().PlotDefaultSize.y, 0.0f, 1000.0f);
}

} // namespace SBCQueens