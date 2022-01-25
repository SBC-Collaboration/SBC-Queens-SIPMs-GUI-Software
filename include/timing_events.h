#pragma once

#include <cassert>
#include <chrono>
#include <cmath>
#include <utility>
#include <thread>
#include <optional>
#include <type_traits>

#include <spdlog/spdlog.h>

namespace SBCQueens {

	template<typename Y = std::chrono::milliseconds>
	double get_current_time_epoch() {
		return static_cast<double>(
			std::chrono::duration_cast<Y>(
				std::chrono::high_resolution_clock::now().time_since_epoch()
			).count()
		);
	}

	// TotalTimeEvent
	// Makes sure the function passed to it with arguments args
	// is only called 
	template<typename FuncName, typename Y = std::chrono::milliseconds>
	class TotalTimeEvent {
	public:

		using time_format = Y;

		TotalTimeEvent(FuncName&& f, const time_format& total_time) 
			: _needToCall(true), _total_wait_time(total_time), _f(f) {  };

		template<typename... Args>
		auto operator()(Args&&... args) {

			// There are two cases: if f returns void or not
			// If it returns void, just call it and thats it.
			if constexpr ( std::is_void_v<std::result_of_t<FuncName&&(Args&&...)>> ) {
				// If function needs to be called, it:
				// 1.- Saves init time
				// 2.- Runs the function
				// 3.- Sets needToCall to false
				// 4.- Wrap the return of the function in std::optional
				if(_needToCall) {
					_init_time = std::chrono::high_resolution_clock::now();

					_f(std::forward<Args>(args)...);

					_needToCall = false;
				}

				_final_time = std::chrono::high_resolution_clock::now();

				// calculate the time it has elapsed since last the function was called
				_elapsed_time = std::chrono::duration_cast<time_format>(_final_time - _init_time);

				if(_elapsed_time >= _total_wait_time) {
					_needToCall = true;
				} 

			} else {
				// If function does not return in this call, optional is empty
				std::optional<std::invoke_result_t<FuncName, Args...>> return_of_func = {};

				// If function needs to be called, it:
				// 1.- Saves init time]
				// 2.- Runs the function
				// 3.- Sets needToCall to false
				// 4.- Wrap the return of the function in std::optional
				if(_needToCall) {
					_init_time = std::chrono::high_resolution_clock::now();

					auto r = _f(std::forward<Args>(args)...);

					_needToCall = false;
					// return the same thing as f(args)
					return_of_func = std::make_optional(r);
				}

				_final_time = std::chrono::high_resolution_clock::now();

				// calculate the time it has elapsed since last the function was called
				_elapsed_time = std::chrono::duration_cast<time_format>(_final_time - _init_time);

				if(_elapsed_time >= _total_wait_time) {
					_needToCall = true;
				} 

				return return_of_func;
			}
			

		}

	private:
		bool _needToCall;
		time_format _total_wait_time;
		FuncName _f;

		std::chrono::high_resolution_clock::time_point _init_time;
		std::chrono::high_resolution_clock::time_point _final_time;
		time_format _elapsed_time;
	};

	// Wrapper function that hides all the nasty bits and turns your function into
	// a timed event function!
	template<typename FuncName, typename Y = std::chrono::milliseconds>
	inline TotalTimeEvent<FuncName, Y> make_total_timed_event(const Y& total_time, FuncName&& f) {

		return TotalTimeEvent<FuncName, Y>{std::forward<FuncName>(f), total_time};

	}

	// BlockingTotalTimeEvent
	// Makes sure that the total execution time of the function passed to it
	// equals the time it was assigned to.
	// If the execution time of the function is higher than the function passed to it
	// it is essentially ignored.
	template<typename FuncName, typename Y = std::chrono::milliseconds>
	class BlockingTotalTimeEvent {
	public:

		using time_format = Y;

		BlockingTotalTimeEvent(FuncName&& f, const time_format& total_time) : _total_wait_time(total_time), _f(f) {  };

		template<typename... Args>
		auto operator()(Args&&... args) {

			// There are two cases: if f returns void or not
			// If it returns void, just call it and thats it.
			if constexpr (std::is_void_v<std::result_of_t<FuncName&&(Args&&...)>> ) {

				_init_time = std::chrono::high_resolution_clock::now();

				_f(std::forward<Args>(args)...);

				_final_time = std::chrono::high_resolution_clock::now();

							// elapsed_time should be the same type as current
				elapsed_time = std::chrono::duration_cast<time_format>(_final_time - _init_time);
				if(elapsed_time < _total_wait_time) {
					//spdlog::info("Sleeping for {0}", (_total_wait_time - elapsed_time).count());
					std::this_thread::sleep_for(_total_wait_time - elapsed_time);
				} 
			// If it does not, return the rest
			} else {
				_init_time = std::chrono::high_resolution_clock::now();

				auto r = _f(std::forward<Args>(args)...);

				_final_time = std::chrono::high_resolution_clock::now();

							// elapsed_time should be the same type as current
				elapsed_time = std::chrono::duration_cast<time_format>(_final_time - _init_time);
				if(elapsed_time < _total_wait_time) {
					//spdlog::info("Sleeping for {0}", (_total_wait_time - elapsed_time).count());
					std::this_thread::sleep_for(_total_wait_time - elapsed_time);
				} 

				return r;
			}
		}

	private:
		time_format _total_wait_time;
		FuncName _f;

		std::chrono::high_resolution_clock::time_point _init_time;
		std::chrono::high_resolution_clock::time_point _final_time;
		time_format elapsed_time;

	};

	template<typename FuncName, typename Y = std::chrono::milliseconds>
	inline BlockingTotalTimeEvent<FuncName, Y> make_blocking_total_timed_event(const Y& total_time, FuncName&& f) {

		return BlockingTotalTimeEvent<FuncName, Y>{std::forward<FuncName>(f), total_time};

	}

	// BlockingFixedTimeEvent
	// Waits for time, total_time, after function f has been executed
	// with arguments args
	template<typename FuncName, typename Y = std::chrono::milliseconds>
	class BlockingFixedTimeEvent {
	public:

		using time_format = Y;

		BlockingFixedTimeEvent(FuncName&& f, const Y& total_time) : _total_wait_time(total_time), _f(f) {  };

		template<typename... Args>
		auto operator()(Args&&... args) const {

			if constexpr (std::is_void_v<std::result_of_t<FuncName&&(Args&&...)>> ) {
				_f(std::forward<Args>(args)...);
				std::this_thread::sleep_for(_total_wait_time);
			} else {
				auto r = _f(std::forward<Args>(args)...);
				std::this_thread::sleep_for(_total_wait_time);
				return r;
			}
			

		}

	private:
		time_format _total_wait_time;
		FuncName _f;
	};

	template<typename FuncName, typename Y = std::chrono::milliseconds>
	inline BlockingFixedTimeEvent<FuncName, Y> make_blocking_fixed_timed_event(const Y& total_time, FuncName&& f) {

		return BlockingFixedTimeEvent<FuncName, Y>{std::forward<FuncName>(f), total_time};

	}


} // namespace SBCQueens