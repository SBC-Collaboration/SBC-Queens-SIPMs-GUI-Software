#pragma once

// STD includes
#include <atomic>
#include <exception>
#include <fstream>
#include <future>
#include <ios>
#include <memory>
#include <string>
#include <type_traits>
#include <filesystem>

// 3rd party includes
#include <concurrentqueue.h>

namespace SBCQueens {

	// This is just to let the programmer (or idiot me) that
	// this file is going to be used to write the data files
	// No reading allowed
	template <typename T>
	struct dataFile {
private:
		// Path to the file
		bool _open;
		std::string _fullFileDir;
		std::ofstream _stream;

		moodycamel::ConcurrentQueue<T> _queue;

public:
		using type = T;

		dataFile() : _open(false) { }

		// FileName: is the relative or absolute file dir
		explicit dataFile(const std::string& fileName) {

			_fullFileDir = fileName;
			_stream.open(_fullFileDir,
				std::ofstream::app | std::ofstream::binary);

			if(_stream.is_open()) {
				_open = true;
			}
			
		}

		template<typename InitWriteFunc, typename... Args>
		// FileName: is the relative or absolute file dir
		// f -> function that takes args and returns string
		// that is written at the start of the file only if
		// the file did not exist before
		dataFile(const std::string& fileName, InitWriteFunc&& f, Args&&... args) {

			_fullFileDir = fileName;
			_stream.open(_fullFileDir,
				std::ofstream::app | std::ofstream::binary);

			if(_stream.is_open()) {
				_open = true;

				if(std::filesystem::is_empty(fileName)) {
					_stream << f(std::forward<Args>(args)...);
				}

			}

		}

		// No copying nor moving
		dataFile(dataFile&&) = delete;
		dataFile(const dataFile&) = delete;

		// It is very likely these two things are happening anyways when
		// the Datafile is out of the scope but I want to make sure.
		~dataFile() {
			_stream.close();
		}

		bool IsOpen() const {
			return _open;
		}

		auto GetData() {
			auto approx_length = _queue.size_approx();
			auto data = std::vector< T >(approx_length);
			_queue.try_dequeue_bulk(data.data(), approx_length);

			return data;
		}

		// Adds element as a copy to current buffer
		void Add(const T& element) {
			_queue.enqueue(element);
		}

		// Adds list as a copy to current buffer
		void Add(const std::initializer_list<T>& list) {
			_queue.enqueue_bulk(list.begin(), list.size());
		}

		// Adds element as a copy to current buffer
		void operator()(const T& element) {
			Add(element);
		}

		// Adds list as a copy to current buffer
		void operator()(const std::initializer_list<T>& list) {
			Add(list);
		}

		// Saves string to the file
		template <typename DATA>
		void operator<<(const DATA& fmt) {
			_stream << fmt;
		}

	};

	template<typename T>
	using DataFile = std::unique_ptr<dataFile<T>>;

	template <typename T>
	void open(DataFile<T>& res, const std::string& fileName) {

		if(res) {
			res.release();
		}

		res = std::make_unique<dataFile<T>>(fileName);

	}

	template <typename T, typename InitWriteFunc, typename... Args>
	void open(DataFile<T>& res,
		const std::string& fileName, InitWriteFunc&& f,  Args&&... args) {

		if(res) {
			res.release();
		}

		res = std::make_unique<dataFile<T>>(fileName,
			std::forward<InitWriteFunc>(f), std::forward<Args>(args)...);
	}

	template<typename T, typename FormatFunc, typename... Args>
	// Saves the contents of DataFile using the format function f
	// that takes T as an argument, or
	// takes std::deque<T>& as argument which then uses all the information
	// at once to generate the string that is saved to the file
	void save(DataFile<T>& file, FormatFunc&& f,  Args&&... args) noexcept {

		if(file->IsOpen()) {
			// GetData becomes a thread-safe operation
			// because of the concurrent queue
			// for the async version this data is locked into the
			// thread that is saving and is cleared by the end
			auto data = file->GetData();
			// If FormatFunc takes the single item as an argument
			// We will call f for every item in TempData
			if constexpr (
				std::is_invocable_v<FormatFunc, T, Args...> 		||
				std::is_invocable_v<FormatFunc, const T&, Args...> 	||
				std::is_invocable_v<FormatFunc, T&, Args...>) {

				for(auto item : data) {
					(*file) << f(item, std::forward<Args>(args)...);
				}

			// If it takes the entire format, then apply it to all.
			} else if constexpr (std::is_invocable_v<FormatFunc,
				std::vector<T>&, Args...>) {

				(*file) << f(data, std::forward<Args>(args)...);

			// if not, just save what f returns
			} else {
				(*file) << f(std::forward<Args>(args)...);
			}
		}

	}

	template<typename T, typename FormatFunc, typename... Args>
	// Saves the contents of the DataFile asynchronously using packaged tasks
	void async_save(DataFile<T>& file, FormatFunc&& f, Args&&... args) noexcept {
		// Here is where ToggleDataBuffer() is important.
		// During an async there should be two arrays:
		// One to write to, and another to read from.
		std::packaged_task<void(DataFile<T>&, FormatFunc&&)>
			_pt(save<T, FormatFunc>);
			
		_pt(file, std::forward<FormatFunc>(f), std::forward<Args>(args)...);
	}



} //namespace SBCQueens
