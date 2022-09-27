

// C++ STD includes
#include <chrono>
#include <thread>

// C++ 3rd party includes
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <readerwriterqueue.h>

// My includes
// Only have 1 include at a time!
#ifdef USE_VULKAN
#include "rendering_wrappers/glfw_vulkan_wrapper.h"
#else
#include "rendering_wrappers/glfw_opengl3_wrapper.h"
#endif

// my includes
#include "include/imgui_helpers.hpp"

#include "TeensyControllerInterface.hpp"
#include "CAENDigitizerInterface.hpp"
#include "SlowDAQInterface.hpp"
#include "GUIManager.hpp"

int main() {
    // try{
    //     auto async_file = spdlog::rotating_logger_mt<spdlog::async_factory>(
    //      "main logger", "log.txt", 1024 * 1024 * 5, 3
    //  );

    //  spdlog::set_default_logger(async_file);
    // } catch(const spdlog::spdlog_ex& ex) {
    //   std::cout << "Log initialization failed: " << ex.what() << std::endl;
    // }

    spdlog::info("Starting software");
    SBCQueens::TeensyQueue teensyQueue;
    SBCQueens::CAENQueue caenQueue;
    SBCQueens::SlowDAQQueue otherQueue;
    SBCQueens::GeneralIndicatorQueue giQueue;
    SBCQueens::MultiplePlotQueue mpQueue;
    SBCQueens::SiPMPlotQueue sipmQueue;

    // This is our GUI function which actually holds all of our buttons
    // labels, inputs, graphs and ect
    SBCQueens::GUIManager guiManager(
        // From GUI -> Teensy
        teensyQueue,
        // From GUI -> CAEN
        caenQueue,
        // From GUI -> slow DAQ
        otherQueue,
        // From Anyone -> GUI
        giQueue,
        // From Anyone -> GUI, auxiliary queue for dynamic plots
        mpQueue,
        // From CAEN -> GUI, auxiliary queue for dynamic plots
        sipmQueue);

    spdlog::info("Creating wrapper. Using:");


#ifdef USE_VULKAN
    spdlog::info("VULKAN + GLFW");
#else
    spdlog::info("OpenGL + GLFW");
#endif

    // This function just holds the rendering framework we are using
    // all of them found under rendering_wrappers
    // We wrapping it under a lambda so we can pass it to a thread
    // because we do not care about its returning vallue
    auto gui_wrapper = [&]() {
#ifdef USE_VULKAN
        ImGUIWrappers::main_glfw_vulkan_wrapper(guiManager);
#else
        ImGUIWrappers::main_glfw_open3gl_wrapper(guiManager);
#endif
    };

    // std::thread main_thread(gui_wrapper);

    // The lambdas we are passing are the functions
    // to read and write to the queue
    SBCQueens::TeensyControllerInterface tc(
        // GUI -> Teensy
        teensyQueue,
        // From Anyone -> GUI
        giQueue,

        mpQueue);

    std::thread tc_thread(std::ref(tc));

    SBCQueens::CAENDigitizerInterface caenc(
        giQueue,
        caenQueue,
        mpQueue,
        sipmQueue);

    std::thread caen_thread(std::ref(caenc));

    SBCQueens::SlowDAQInterface otherc(
        giQueue,
        otherQueue,
        mpQueue);

    std::thread other_thread(std::ref(otherc));

    spdlog::info("Starting threads");

    tc_thread.detach();
    caen_thread.detach();
    other_thread.detach();
    gui_wrapper();

    // while(!tc_thread.joinable());
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    spdlog::info("Closing ! ! !");

    return 0;
}
