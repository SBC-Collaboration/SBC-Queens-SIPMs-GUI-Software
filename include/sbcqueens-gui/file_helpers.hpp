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

template<typename T>
concept HasOutstreamOp = requires (T x) { x.operator<<; };

// Manages the ofstream to work in concurrent or non-concurrent modes.
// Closes the file when this class goes out of scope. It is possible
// to close the file without destroying this class, but it is not
// recommended.
template <typename T>
requires std::copyable<T> or std::movable<T>
class DataFile {
    bool _open = false;
    std::string _abs_dir;
    std::ofstream _stream;

    moodycamel::ConcurrentQueue<T> _queue;

 public:
    using type = T;

    DataFile() = default;

    // Opens file in append mode. If it does not exist, it creates it.
    // It opens the file with no header.
    explicit DataFile(std::string_view file_name) : _abs_dir(file_name) {
        _stream.open(_abs_dir,
            std::ofstream::app | std::ofstream::binary);

        if (_stream.is_open()) {
            _open = true;
        }
    }

    // Opens file in append mode. If it does not exist, it creates it.
    // It writes the return contents of InitWriteFunc as the header
    // with args...
    template<typename HeaderFunc, typename... HeaderFuncArgs>
    requires HasOutstreamOp<std::invoke_result<HeaderFunc(HeaderFuncArgs...)>>
    DataFile(std::string_view file_name, HeaderFunc&& f, HeaderFuncArgs&&... args) :
        DataFile(file_name)
    {
        if (_open) {
            if (std::filesystem::is_empty(file_name)) {
                _stream << f(std::forward<Args>(args)...);
            }
        }
    }

    virtual ~DataFile() {
        // It is very likely that the stream is closed and freed when this
        // goes out of scope, but we are never too sure.
        _stream.close();
        _open = false;
    }

    const bool& isOpen() noexcept { return _open; }

    std::vector<T> getData() noexcept {
        auto approx_length = _queue.size_approx();
        auto data = std::vector<T>(approx_length);
        _queue.try_dequeue_bulk(data.data(), approx_length);

        return data;
    }

    // Adds element as a copy to current buffer
    void add(const T& element) noexcept {
        _queue.enqueue(element);
    }

    // Adds list as a copy to current buffer
    void add(const std::initializer_list<T>& list) noexcept {
        _queue.enqueue_bulk(list.begin(), list.size());
    }

    // Adds element as a copy to current buffer
    void operator()(const T& element) noexcept {
        add(element);
    }

    // Adds list as a copy to current buffer
    void operator()(const std::initializer_list<T>& list) noexcept {
        add(list);
    }

    // Saves string to the file
    template <typename DATA>
    void operator<<(const DATA& fmt) noexcept {
        _stream << fmt;
    }

    // Flush the buffer to file
    void flush() noexcept {
        _stream.flush();
    }

    // Closes the file
    void close() noexcept {
        _stream.close();
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
    template<typename FormatFunc, typename... Args>
    requires HasOutstreamOp<std::invoke_result<FormatFunc(T, Args...)>> or
             HasOutstreamOp<std::invoke_result<FormatFunc(std::vector<T>, Args...)>> or
             HasOutstreamOp<std::invoke_result<FormatFunc(Args...)>>
    void save(FormatFunc&& f,  Args&&... args) noexcept {
        if(not _open) {
            return;
        }

        // GetData becomes a thread-safe operation
        // because of the concurrent queue
        // for the async version this data is locked into the
        // thread that is saving and is cleared by the end
        auto data = getData();
        // If FormatFunc takes the single item as an argument
        // We will call f for every item in TempData
        if constexpr (
                std::is_invocable_v<FormatFunc, T, Args...>         or
                std::is_invocable_v<FormatFunc, const T&, Args...>  or
                std::is_invocable_v<FormatFunc, T&, Args...>) {
            for (auto item : data) {
                _stream << f(item, std::forward<Args>(args)...);
            }

            // If it takes the entire format, then apply it to all.
        } else if constexpr (std::is_invocable_v<FormatFunc,
                std::vector<T>&, Args...>) {
            _stream << f(data, std::forward<Args>(args)...);

            // if not, just save what f returns
        } else {
            _stream << f(std::forward<Args>(args)...);
        }

        _stream.flush();
    }
};

// Opens the file with name file_name.
// Essentially the same as calling DataFile<T>(file_name)
template <typename T>
DataFile<T> open_file(std::string_view file_name) noexcept {
    return DataFile<T>(file_name);
}

// Opens the file with name fileName.
// Essentially the same as calling DataFile<T>(file_name, f, args)
template <typename T, typename InitWriteFunc, typename... Args>
DataFile<T> open_file(DataFile<T>& res,
                      std::string_view file_name,
                      InitWriteFunc&& f,
                      Args&&... args) noexcept {
    return DataFile<T>(file_name,
                       std::forward<InitWriteFunc>(f),
                       std::forward<Args>(args)...);
}



// Saves the contents of the DataFile asynchronously using the save function
// described here. See save for more information.
template<typename T, typename FormatFunc, typename... Args>
void async_save(DataFile<T>& file, FormatFunc&& f, Args&&... args) noexcept {
    std::packaged_task<void(DataFile<T>&, FormatFunc&&)>
        _pt(save<T, FormatFunc>);

    _pt(file, std::forward<FormatFunc>(f), std::forward<Args>(args)...);
}

struct SaveFileInfo {
    double Time;
    std::string FileName;

    SaveFileInfo() = default;
    SaveFileInfo(const double& time, const std::string& fn) :
        Time(time), FileName(fn) {}
};

// A simplification of DataFile for logging.
using LogFile = DataFile<SaveFileInfo>;

}  // namespace SBCQueens
#endif
