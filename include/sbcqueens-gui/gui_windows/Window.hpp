#ifndef WINDOW_H
#define WINDOW_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ std includes
#include <vector>
// C++ 3rd party includes
// my includes

namespace SBCQueens {

template<typename Pipes>
class Tabs {

};

template<typename Pipes>
class Window {
 protected:
    Pipes _pipes;
    std::vector<Tabs<Pipes>> _tabs;
    virtual void draw() = 0;

 public:
    explicit Window(const Pipes& p) : _pipes{p} {}
    virtual ~Window() = 0;

    bool operator()() {
        draw();

        for(auto& tab : _tabs) {
            tab();
        }
    }


};

} // namespace SBCQueens

#endif