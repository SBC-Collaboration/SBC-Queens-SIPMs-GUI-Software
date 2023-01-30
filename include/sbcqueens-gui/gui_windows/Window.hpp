#ifndef WINDOW_H
#define WINDOW_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ std includes
#include <vector>
#include <string>

// C++ 3rd party includes
#include <imgui.h>
#include <toml.hpp>

// my includes

namespace SBCQueens {

template<typename Pipes>
class Tab {
 protected:
    Pipes _pipes;
    std::string _name;
    virtual void draw() = 0;
    virtual void init_tab(const toml::table& cfg) = 0;

 public:
    explicit Tab(const Pipes& p, const std::string& name) :
        _pipes{p}, _name{name} {}
    virtual ~Tab() = 0;

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

template<typename Pipes>
class Window {
 protected:
    Pipes _pipes;
    std::string _name;
    std::vector<Tab<Pipes>> _tabs;
    virtual void draw() = 0;
    virtual void init_window(const toml::table& cfg) = 0;

 public:
    explicit Window(const Pipes& p, const std::string& name) :
        _pipes{p}, _name{name} {}
    virtual ~Window() = 0;

    void init(const toml::table& cfg) {
        init_window(cfg);

        for(auto& tab: _tabs) {
            tab.init(cfg);
        }
    }

    void operator()() {
        if (not ImGui::Begin(_name.c_str())) {
            ImGui::End();
            return;
        }

        draw();

        if (ImGui::BeginTabBar(_name.c_str())) {
            for(auto& tab : _tabs) {
                tab();
            }
        }

        ImGui::End();
    }
};

} // namespace SBCQueens

#endif