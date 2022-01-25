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
	struct DataFile {
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
		// Const variblae to indicate this is will only append
		//const std::ios_base::openmode Mode = std::ofstream::app;

		DataFile() : _open(false) { }

		// FileName: is the relative or absolute file dir
		explicit DataFile(const std::string& fileName) {

			_fullFileDir = fileName;

			_stream.open(_fullFileDir, std::ofstream::app);

			if(!_stream.is_open()) {
				throw std::runtime_error("Failed to open the file");
			}

			_current_data = std::make_shared<std::deque<T>>(_data);
			_backup_data = std::make_shared<std::deque<T>>(_other_data);
			_open = true;
			
		}

		// Allow moving
		DataFile(DataFile&&) = default;
		DataFile& operator=(DataFile&&) = default;

		// No copying
		DataFile(const DataFile&) = delete;

		// It is very likely these two things are happening anyways when
		// the Datafile is out of the scope but I want to make sure.
		~DataFile() {
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
			_backup_data->push_back(element);
		}

		// Adds list as a copy to current buffer
		void Add(std::initializer_list<T>& list) {
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
		void operator()(std::initializer_list<T>& list) {
			Add(list);
		}

		// Saves string to the file
		void operator<<(const std::string& fmt) {

			_stream << fmt;

		}

		template <typename FormatFunc>
		std::string operator|(FormatFunc f) {
			return f(_current_data.get());
		}

	};

	template <typename T>
	bool open(DataFile<T>& f,const std::string& fileName) {
		// This should close the file and release any resources
		try{
			f = DataFile<T>(fileName);
			return true;
		} catch (...) {
			return false;
		}
	}

	template<typename T, typename FormatFunc>
	// Saves the contents of DataFile
	void save(DataFile<T>& file, FormatFunc&& f) noexcept {

		// If FormatFunc takes the single item as an argument
		// We will call f for every item in TempData
		if constexpr (std::is_invocable_v<FormatFunc, T>) {
			if(file.IsOpen()) {
				for(auto item : file.GetWritingData()) {
					std::string fmt_item = f(item);
					file << fmt_item;
				}

				file.Clear();
			}

		// If it takes the entire format, then apply it to all.
		} else if constexpr (std::is_invocable_v<FormatFunc, const DataFile<T>&>) {

			if(file.IsOpen()) {
				std::string str = file | f;
				file << str;
			}

		}

	}

	template<typename T, typename FormatFunc>
	// Saves the contents of the DataFile asynchronously using packaged tasks
	void async_save(DataFile<T>& file, FormatFunc&& f) noexcept {
		// Here is where ToggleDataBuffer() is important.
		// During an async there should be two arrays:
		// One to write to, and another to read from.
		file.ToggleDataBuffer();
		std::packaged_task<void(DataFile<T>&, FormatFunc&&)>
			_f(save<T, FormatFunc>);
			
		_f(file, std::forward<FormatFunc>(f));
	}

} //namespace SBCQueens
