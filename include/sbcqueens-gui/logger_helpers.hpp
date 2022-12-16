#ifndef LOGGER_HELPERS_H
#define LOGGER_HELPERS_H
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <iostream>
// C++ 3rd party includes
// my includes

namespace SBCQueens {

class iostream_wrapper {
 public:
    iostream_wrapper() {}

    template<typename ...Args>
    void print(const std::string& out, Args&&... args) {
        std::cout << out << std::endl;
    }

    template<typename ...Args>
    void debug(const std::string& out, Args&&... args) {
        print(out);
    }

    template<typename ...Args>
    void warn(const std::string& out, Args&&... args) {
        print(out);
    }

    template<typename ...Args>
    void log(const std::string& out, Args&&... args) {
        print(out);
    }

    template<typename ...Args>
    void error(const std::string& out, Args&&... args) {
        print(out);
    }
};

}  // namespace SBCQueens

#endif