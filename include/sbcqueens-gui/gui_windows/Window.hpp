#ifndef WINDOW_H
#define WINDOW_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ std includes
#include <vector>
#include <string>
#include <memory>

// C++ 3rd party includes
#include <imgui.h>
#include <toml.hpp>

// my includes

namespace SBCQueens {

class TabInterface {
 protected:
    std::string _name;
    explicit TabInterface(std::string_view name) : _name{name} {}

 public:
    virtual ~TabInterface() {}

    virtual void init(const toml::table& cfg) = 0;
    virtual void operator()() = 0;
};

template<typename... DataTypes>
class Tab : public TabInterface {
 protected:
    virtual void draw() = 0;
    virtual void init_tab(const toml::table& cfg) = 0;

    explicit Tab(std::string_view name) : TabInterface{name} {}

 public:
    virtual ~Tab() {}

    void init(const toml::table& cfg) {
        init_tab(cfg);
    }

    void operator()() {
        if (ImGui::BeginTabItem(_name.c_str())) {
            draw();
            ImGui::EndTabItem();
        }
    }
};

class WindowInterface {
 protected:
    std::string _name;

    explicit WindowInterface(std::string_view name) : _name{name} {}

 public:
    virtual ~WindowInterface() {}
    virtual void init(const toml::table& cfg) = 0;
    virtual void operator()() = 0;
};

template<typename... DataTypes>
class Window : public WindowInterface {
 protected:
    std::vector<std::unique_ptr<TabInterface>> _tabs;
    virtual void draw() = 0;
    virtual void init_window(const toml::table& cfg) = 0;

    explicit Window(std::string_view name) : WindowInterface{name} {}

 public:
    virtual ~Window() {}

    void init(const toml::table& cfg) {
        init_window(cfg);

        for (auto& tab: _tabs) {
            tab->init(cfg);
        }
    }

    void operator()() {
        if (not ImGui::Begin(_name.c_str())) {
            ImGui::End();
            return;
        }

        draw();

        if (ImGui::BeginTabBar(_name.c_str())) {
            for (auto& tab : _tabs) {
                (*tab)();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }
};

} // namespace SBCQueens

#endif