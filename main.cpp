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
#include "OtherDevicesInterface.h"
#include "GUIManager.h"

int main(int argc, char *argv[])
{
	spdlog::info("Starting software");
	SBCQueens::TeensyQueue teensyQueue;
	SBCQueens::CAENQueue caenQueue;
	SBCQueens::OtherQueue otherQueue;
	SBCQueens::GeneralIndicatorQueue giQueue;
	SBCQueens::MultiplePlotQueue mpQueue;

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
		mpQueue
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
		teensyQueue,
		// From Anyone -> GUI
		giQueue,

		mpQueue
	);

	std::thread tc_thread(std::ref(tc));

	SBCQueens::CAENDigitizerInterface caenc(
		giQueue,
		caenQueue,
		mpQueue
	);

	std::thread caen_thread(std::ref(caenc));

	SBCQueens::OtherDevicesInterface otherc(
		giQueue,
		otherQueue,
		mpQueue
	);

	std::thread other_thread(std::ref(otherc));

	spdlog::info("Starting threads");

	tc_thread.detach();
	caen_thread.detach();
	other_thread.detach();
	gui_wrapper();

	spdlog::info("Closing software");

	return 0;
}

