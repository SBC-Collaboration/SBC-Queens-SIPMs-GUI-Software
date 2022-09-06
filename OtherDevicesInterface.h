#pragma once

/*

	This class is intended to be used to hold miscellaneous devices
	which time scales are in seconds or higher.

	Example: pressure transducers

*/

// std includes
#include <string>

// 3rd party includes


// project includes
#include "serial_helper.h"
#include "file_helpers.h"
#include "timing_events.h"

#include "indicators.h"

namespace SBCQueens {

	enum class PFEIFFERSingleGaugeSP {
		SLOW, 	// 1 min
		FAST, 	// 1s
		FASTER	// 100ms
	};

	struct PFEIFFERSingleGaugeData {
		double Pressure;
	};

	struct OtherDevicesData {
		std::string RunDir 		= "";
		std::string RunName 	= "";

		bool PFEIFFERSingleGaugeEnable = false;
		PFEIFFERSingleGaugeSP PFEIFFERSingleGaugeUpdateSpeed
			= PFEIFFERSingleGaugeSP::SLOW;
	};

	using OtherInQueueType
		= std::function < bool(OtherDevicesData&) >;

	using OtherInQueue
		= moodycamel::ReaderWriterQueue< OtherInQueueType >;

	template<typename... Queues>
	class OtherDevicesInterface {

		std::tuple<Queues&...> _queues;

		OtherDevicesData state_of_everything;

		DataFile<PFEIFFERSingleGaugeData> _pfeiffer_file;

		IndicatorSender<IndicatorNames> _plot_sender;

		// PFEIFFER Single Gauge port
		serial_ptr _pfeiffers_port;

	public:

		explicit OtherDevicesInterface(Queues&... queues) :
			_queues(forward_as_tuple(queues...)),
			_plot_sender(std::get<GeneralIndicatorQueue&>(_queues)) {

		}

		// No copying
		OtherDevicesInterface(const OtherDevicesInterface&) = delete;


		void operator()() {
			spdlog::info("Initializing slow DAQ thread...");
			spdlog::info("Slow DAQ components: PFEIFFERSingleGauge");

			// GUI -> Slow Daq
			OtherInQueue& guiQueueOut = std::get<OtherInQueue&>(_queues);
			auto guiQueueFunc = [&guiQueueOut]() -> SBCQueens::OtherInQueueType {

				SBCQueens::OtherInQueueType new_task;
				bool success = guiQueueOut.try_dequeue(new_task);

				if(success) {
					return new_task;
				} else {
					return [](SBCQueens::OtherDevicesData&) { return true; };
				}
			};
		}

	};

} //namespace SBCQueens
