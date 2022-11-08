// C STD includes
// C 3rd party includes
// C++ STD include
// C++ 3rd party includes
#include <doctest/doctest.h>

#include <chrono>
#include <thread>
#include <optional>
#include <iostream>
#include <memory>

#include "sbcqueens-gui/hardware_helpers/ClientController.hpp"

TEST_CASE("GUI_TEST") {

    SBCQueens::ClientController<std::unique_ptr<int>, double> fake_port("test");

    fake_port.init(
    [](std::unique_ptr<int>& i) -> bool{
        i.reset(new int);
        *i = 2;
        return true;
    }, [](std::unique_ptr<int>& i) -> bool {
        i.release();
        return true;
    });

    fake_port.async_get([](std::unique_ptr<int>& i) -> std::optional<double> {
        *i = 42;

        std::this_thread::sleep_for(std::chrono::seconds(1));

        return std::make_optional(static_cast<double>(*i));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    fake_port.async_get([](std::unique_ptr<int>& i) -> std::optional<double> {
        *i = 69;

        std::this_thread::sleep_for(std::chrono::seconds(1));

        return std::make_optional(static_cast<double>(*i));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    auto output = fake_port.async_get_values();
    for(auto x : output) {
        std::cout << x << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(3));

    std::cout << "We waited..." << std::endl;
    output = fake_port.async_get_values();
    for(auto x : output) {
        std::cout << x << std::endl;
    }

}