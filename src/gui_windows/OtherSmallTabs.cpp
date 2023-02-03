#include "sbcqueens-gui/gui_windows/OtherSmallTabs.hpp"

namespace SBCQueens {

void GuiConfigTab::init_tab(const toml::table&) {}

void GuiConfigTab::draw() {
	ImGui::SliderFloat("Plot line-width",
	    &ImPlot::GetStyle().LineWeight, 0.0f, 10.0f);
	ImGui::SliderFloat("Plot Default Size Y",
	    &ImPlot::GetStyle().PlotDefaultSize.y, 0.0f, 1000.0f);
}

} // namespace SBCQueens