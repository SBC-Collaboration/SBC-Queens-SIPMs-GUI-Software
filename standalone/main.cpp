// C++ STD includes
#include <chrono>
#include <thread>

// C++ 3rd party includes
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <readerwriterqueue.h>

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"

// My includes
// Only have 1 include at a time!
#ifdef USE_VULKAN
#include "sbcqueens-gui/rendering_wrappers/glfw_vulkan_wrapper.h"
#else
#include "sbcqueens-gui/rendering_wrappers/glfw_opengl3_wrapper.h"
#endif

// my includes
#include "sbcqueens-gui/imgui_helpers.hpp"

#include "sbcqueens-gui/hardware_helpers/TeensyControllerInterface.hpp"
#include "sbcqueens-gui/hardware_helpers/CAENDigitizerInterface.hpp"
#include "sbcqueens-gui/hardware_helpers/SlowDAQInterface.hpp"
#include "sbcqueens-gui/hardware_helpers/GUIManager.hpp"

int main() {
    try{
        spdlog::init_thread_pool(8192, 1);
        auto stdout_sink
            = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();
        auto rotating_sink
            = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                "log.txt", 1024*1024*10, 3);
        std::vector<spdlog::sink_ptr> sinks {stdout_sink, rotating_sink};
        auto logger = std::make_shared<spdlog::async_logger>("log",
            sinks.begin(), sinks.end(), spdlog::thread_pool(),
            spdlog::async_overflow_policy::block);
        spdlog::register_logger(logger);

    } catch(const spdlog::spdlog_ex& ex) {
      std::cout << "Log initialization failed: " << ex.what() << std::endl;
      throw "Log initialization failed";
    }

    spdlog::info("Starting software by first initializing all the queues.");
    //TODO(Hector): these should be changed to shared_ptrs
    SBCQueens::TeensyQueue teensyQueue;
    SBCQueens::CAENQueue caenQueue;
    SBCQueens::SlowDAQQueue otherQueue;
    SBCQueens::GeneralIndicatorQueue giQueue;
    SBCQueens::MultiplePlotQueue mpQueue;
    SBCQueens::SiPMPlotQueue sipmQueue;

    spdlog::info("Then the GUI.");
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
    spdlog::info("Using VULKAN + GLFW backend");
#else
    spdlog::info("Usigng OpenGL + GLFW backend");
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
    spdlog::info("Creating the teensy interface and CAEN interface.");

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

    spdlog::info("Starting threads which holds everything.");

    tc_thread.detach();
    caen_thread.detach();
    other_thread.detach();
    gui_wrapper();

    // while(!tc_thread.joinable());
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    spdlog::info("Closing ! ! !");

    return 0;
}
