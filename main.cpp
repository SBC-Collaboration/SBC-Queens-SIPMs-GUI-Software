// STL includes
#include <thread>

// 3rd party includes
#include <spdlog/spdlog.h>
#include <readerwriterqueue.h>

// My includes
// Only have 1 include at a time!
#ifdef USE_VULKAN
#include "rendering_wrappers/glfw_vulkan_wrapper.h"
#else
#include "rendering_wrappers/glfw_opengl3_wrapper.h"
#endif

#include "imgui_helpers.h"

#include "TeensyControllerInterface.h"
#include "CAENDigitizerInterface.h"
#include "GUIManager.h"

int main(int argc, char *argv[])
{
	spdlog::info("Starting software");
	SBCQueens::TeensyInQueue guiQueueOut;
	SBCQueens::CAENQueue caenQueue;
	SBCQueens::SiPMsPlotQueue plotQueue;

	// This is our GUI function which actually holds all of our buttons
	// labels, inputs, graphs and ect
	SBCQueens::GUIManager guiManager(
		// From GUI -> Teensy
		guiQueueOut, 
		// From GUI -> CAEN
		caenQueue,
		// From Anyone -> GUI
		plotQueue
	);
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
		guiQueueOut,
		// From Anyone -> GUI
		plotQueue
	);

	std::thread tc_thread(std::ref(tc));

	SBCQueens::CAENDigitizerInterface caenc(
		plotQueue,
		caenQueue
	);

	std::thread caen_thread(std::ref(caenc));

	spdlog::info("Starting threads");

	tc_thread.detach();
	caen_thread.detach();
	gui_wrapper();

	spdlog::info("Closing software");

	return 0;
}

