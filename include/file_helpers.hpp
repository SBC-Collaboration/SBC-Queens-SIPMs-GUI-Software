#ifndef FILEHELPERS_H
#define FILEHELPERS_H
#pragma once


// C STD includes
// C 3rd party includes
// C++ STD includes
#include <vector>
#include <exception>
#include <fstream>
#include <future>
#include <ios>
#include <memory>
#include <string>
#include <type_traits>
#include <filesystem>
#include <utility>

// C++ 3rd party includes
#include <concurrentqueue.h>
#include <spdlog/spdlog.h>

namespace SBCQueens {

// Manages the ofstream to work in concurrent or non-concurrent modes.
// Do not use on its own.
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

        if (_stream.is_open()) {
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

        if (_stream.is_open()) {
            _open = true;

            // TODO(Hector): create directories between file and
            // current directory?
            if (std::filesystem::is_empty(fileName)) {
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
        _open = false;
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

    // Flush the buffer to file
    void flush() {
        _stream.flush();
    }

    // Closes the file
    void close() {
        _stream.close();
    }
};

// As a file is a dynamic resource, it is better to manage it using
// smart pointers, and using a functional based approach it is easier
// to parallelize it.
template<typename T>
using DataFile = std::unique_ptr<dataFile<T>>;

// Opens the file with name fileName.
// If file is an already opened file, it is closed and opened again.
// res returns nullptr if it failed to open.
template <typename T>
void open(DataFile<T>& res, const std::string& fileName) {
    if (res) {
        res.release();
    }

    res = std::make_unique<dataFile<T>>(fileName);

    // If it fails to open, release resources
    if (not res->IsOpen()) {
        res.release();
    }
}

// Opens the file with name fileName.
// If file is an already opened file, it is closed and opened again.
// It will also write to the file whatever f returns with args...
// as long as f returns anything that can be saved using the steam << operator.
// res returns nullptr if it failed to open.
template <typename T, typename InitWriteFunc, typename... Args>
void open(DataFile<T>& res,
    const std::string& fileName, InitWriteFunc&& f,  Args&&... args) {

    if (res) {
        res.release();
    }

    res = std::make_unique<dataFile<T>>(fileName,
        std::forward<InitWriteFunc>(f), std::forward<Args>(args)...);

    // If it fails to open, release resources
    if (not res->IsOpen()) {
        res.release();
    }
}

// Closes the file and releases resources (res is nullptr after this call)
template <typename T>
void close(DataFile<T>& res) {
    if (res) {
        res.release();
    }
}

// Saves the contents of DataFile using three different methods depending on f:
//
// If f takes T (T& or const T) as an argument, it will write the return of
//      f for each data member in the vector.
// Else if f takes std::vector<T>& as argument, it will then use all the
//      information contained inside the vector before writing f.
// Else, write what f returns.
// Note: as long as the return type is compatible with the << operator
// for all of these cases.
template<typename T, typename FormatFunc, typename... Args>
void save(DataFile<T>& file, FormatFunc&& f,  Args&&... args) noexcept {
    if (file->IsOpen()) {
        // GetData becomes a thread-safe operation
        // because of the concurrent queue
        // for the async version this data is locked into the
        // thread that is saving and is cleared by the end
        auto data = file->GetData();
        // If FormatFunc takes the single item as an argument
        // We will call f for every item in TempData
        if constexpr (
            std::is_invocable_v<FormatFunc, T, Args...>         ||
            std::is_invocable_v<FormatFunc, const T&, Args...>  ||
            std::is_invocable_v<FormatFunc, T&, Args...>) {
            for (auto& item : data) {
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

        file->flush();
    }
}

// Saves the contents of the DataFile asynchronously using the save function
// described here. See save for more information.
template<typename T, typename FormatFunc, typename... Args>
void async_save(DataFile<T>& file, FormatFunc&& f, Args&&... args) noexcept {
    std::packaged_task<void(DataFile<T>&, FormatFunc&&)>
        _pt(save<T, FormatFunc>);

    _pt(file, std::forward<FormatFunc>(f), std::forward<Args>(args)...);
}

}  // namespace SBCQueens
#endif
