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
#include <utility>
#include <deque>
#include <stdexcept>
#include <optional>

// 3rd party includes
#include <spdlog/spdlog.h>

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

		std::deque<T> _data;
		std::deque<T> _other_data;
		std::shared_ptr<std::deque<T>> _current_data;
		std::shared_ptr<std::deque<T>> _backup_data;

public:
		using type = T;

		dataFile() : _open(false) { }

		// FileName: is the relative or absolute file dir
		explicit dataFile(const std::string& fileName) {

			_fullFileDir = fileName;
			_stream.open(_fullFileDir, std::ofstream::app);

			_current_data = std::make_shared<std::deque<T>>(_data);
			_backup_data = std::make_shared<std::deque<T>>(_other_data);

			if(_stream.is_open()) {
				_open = true;
			}
			
		}

		// No copying nor moving
		dataFile(dataFile&&) = delete;
		dataFile(const dataFile&) = delete;

		// It is very likely these two things are happening anyways when
		// the Datafile is out of the scope but I want to make sure.
		~dataFile() {
			_stream.close();
			_data.clear();
			_other_data.clear();
		}

		bool IsOpen() const {
			return _open;
		}

		auto GetWritingData() {
			return *_current_data;
		}

		// Toggles the internal buffers
		void ToggleDataBuffer() {
			_current_data.swap(_backup_data);
		}

		// Adds element as a copy to current buffer
		void Add(const T& element) {
			_backup_data->emplace_back(element);
		}

		// Adds list as a copy to current buffer
		void Add(const std::initializer_list<T>& list) {
			_backup_data->insert(_data.end(), list);
		}

		// Clears the buffer that is not in use
		void Clear() {
			_current_data->clear();
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

		template <typename FormatFunc>
		std::string operator|(FormatFunc f) {
			return f(_current_data.get());
		}

	};

	template<typename T>
	using DataFile = std::unique_ptr<dataFile<T>>;

	template <typename T>
	void open(DataFile<T>& f, const std::string& fileName) {
		f = std::make_unique<dataFile<T>>(fileName);
	}

	template<typename T, typename FormatFunc>
	// Saves the contents of DataFile using the format function f
	// that takes T as an argument, or
	// takes std::deque<T>& as argument which then uses all the information
	// at once to generate the string that is saved to the file
	void save(DataFile<T>& file, FormatFunc&& f) noexcept {

		// If FormatFunc takes the single item as an argument
		// We will call f for every item in TempData
		if constexpr (std::is_invocable_v<FormatFunc, T>) {
			if(file->IsOpen()) {
				for(auto item : file->GetWritingData()) {
					std::string fmt_item = f(item);
					(*file) << fmt_item;
				}

				file->Clear();
			}

		// If it takes the entire format, then apply it to all.
		} else if constexpr (std::is_invocable_v<FormatFunc, std::deque<T>&>) {

			if(file->IsOpen()) {
				std::string str = (*file) | f;
				(*file) << str;
			}

		}

	}

	template<typename T, typename FormatFunc>
	// Saves the contents of the DataFile asynchronously using packaged tasks
	void async_save(DataFile<T>& file, FormatFunc&& f) noexcept {
		// Here is where ToggleDataBuffer() is important.
		// During an async there should be two arrays:
		// One to write to, and another to read from.
		file->ToggleDataBuffer();
		std::packaged_task<void(DataFile<T>&, FormatFunc&&)>
			_f(save<T, FormatFunc>);
			
		_f(file, std::forward<FormatFunc>(f));
	}

	template<typename T, typename FormatFunc>
	// Saves the contents of the DataFile asynchronously using packaged tasks
	void sync_save(DataFile<T>& file, FormatFunc&& f) noexcept {
		// Here is where ToggleDataBuffer() is important.
		// During an async there should be two arrays:
		// One to write to, and another to read from.
		file->ToggleDataBuffer();
		save(file, f);
	}

	template<typename T>
	std::string sbc_save_func(std::deque<T>& data) {
		return "";
	}

} //namespace SBCQueens
