//
// Created by Hector Hawley Herrera on 2023-02-19.
//

#ifndef SBCBINARYFORMAT_H
#define SBCBINARYFORMAT_H
#include <tuple>
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
#include <bit>
#include <inttypes.h>
#include <numeric>
#include <fstream>
#include <type_traits>
#include <filesystem>

// C++ 3rd party includes
#include <concurrentqueue.h>
#include <spdlog/spdlog.h>

// my includes
#include "sbcqueens-gui/file_helpers.hpp"

namespace SBCQueens {
namespace BinaryFormat {
namespace Tools {

    // The BinaryFormat only accepts arithmethic pointers.
    // ex: const char* returns true
    //      std::string return false
    //      double* return true;
    template<typename T>
    concept is_arithmethic_ptr = std::is_arithmetic_v<
                                        std::remove_cvref_t<
                                        std::remove_pointer_t<T>>> and std::is_pointer_v<T>;


    template<typename... T>
    concept is_arithmetic_ptr_unpack = requires(T x) {
            (... and is_arithmethic_ptr<T>); };

    template<typename T>
    requires is_arithmethic_ptr<T>
    constexpr static std::string_view type_to_string() {
        // We get the most pure essence of T: no consts, no references,
        // and no []
        using T_no_const = std::remove_pointer_t<std::remove_all_extents_t<
                std::remove_reference_t<std::remove_const_t<T>>>>;
        if constexpr (std::is_same_v<T_no_const, char>) {
            return "char";
        } else if constexpr (std::is_same_v<T_no_const, uint8_t>) {
            return "uint8";
        } else if constexpr (std::is_same_v<T_no_const, uint16_t>) {
            return "uint16";
        } else if constexpr (std::is_same_v<T_no_const, uint32_t>) {
            return "uint32";
        } else if constexpr (std::is_same_v<T_no_const, int8_t>) {
            return "int8";
        } else if constexpr (std::is_same_v<T_no_const, int16_t>) {
            return "int16";
        } else if constexpr (std::is_same_v<T_no_const, int32_t>) {
            return "int32";
        } else if constexpr (std::is_same_v<T_no_const, float>) {
            return "single";
        } else if constexpr (std::is_same_v<T_no_const, double>) {
            return "double";
        } else if constexpr (std::is_same_v<T_no_const, long double>) {
            return "float128";
        }
        // TODO(All): maybe the default should be uint32?
    }

} // namespace Tools

template<typename... DataTypes>
requires Tools::is_arithmetic_ptr_unpack<DataTypes...>
struct DynamicWriter {
    using tuple_type = std::tuple<DataTypes...>;
    constexpr static std::size_t n_cols = sizeof...(DataTypes);
    constexpr static std::array<std::size_t, n_cols> size_of_types = { sizeof(DataTypes)... };
    constexpr static std::array<std::string_view, n_cols> parameters_types_str = { Tools::type_to_string<DataTypes>()... };

private:
    const std::string _file_name;
    const std::array<std::string, n_cols> _names;
    const std::array<std::size_t, n_cols> _ranks;
    const std::vector<std::size_t> _sizes;

    std::size_t total_ranks = 0;
    bool _open = false;
    std::ofstream _stream;

    std::size_t _line_byte_size = 0;
    std::size_t _line_param_order = 0;
    std::size_t _line_buffer_loc = 0;
    std::string _line_buffer;

    moodycamel::ConcurrentQueue<tuple_type> _queue;

    template<typename T>
    void _copy_number_to_buff(const T& num,
                              std::string& buffer,
                              std::size_t& loc,
                              const std::size_t& size = sizeof(T)) {
        const char* tmpstr = reinterpret_cast<const char*>(&num);
        for(std::size_t i = 0; i < size; i++) {
            buffer[i + loc] = tmpstr[size - i - 1];
        }

        loc += size;
    }

    void _copy_str_to_buff(std::string_view source,
                           std::string& buffer,
                           std::size_t& loc) {
        source.copy(&buffer[loc], source.length(), 0);
        loc += source.length();
    }

    void _create_file() {

        /*  SBC Binary Header description:
         * Header of a binary format is divided in 4 parts:
         * 1.- Edianess            - always 4 bits long (uint32_t)
         * 2.- Data Header size    - always 2 bits long (uint16_t)
         * and is the length of the next bit of data
         * 3.- Header              - is data header long.
         * Contains the structure of each line. It is always found as a raw
         * string in the form "{name_col};{type_col};{size1},{size2}...;...;
         * Cannot be longer than 65536 bytes.
         * 4.- Number of lines     - always 4 bits long (int32_t)
         * Number of lines in the file. If 0, it is indefinitely long.
        */
        // Edianess first
        uint32_t endianess = 0x01020304;
        if constexpr (std::endian::native == std::endian::big) {
            endianess = 0x04030201;
        } else {
            endianess = 0x01020304;
        }

        std::size_t total_header_size = 0;
        // has to be uint16_t because we are saving it to the file later
        uint16_t binary_header_size = 0;
        std::size_t total_ranks_so_far = 0;
        for (std::size_t i = 0; i < n_cols; i++) {
            auto column_rank = _ranks[i];
            // + 1 for the ; character
            binary_header_size += _names[i].length() + 1;
            // + 1 for the ; character
            binary_header_size += parameters_types_str[i].length() + 1;

            std::size_t total_rank_size = 0;
            for(std::size_t j = 0; j < column_rank; j ++) {
                auto size = _sizes[total_ranks_so_far + j];
                total_rank_size += size;
                // It is always + 1 because there is either a ',' or a ';'
                binary_header_size += std::to_string(size).length() + 1;
            }

            total_ranks_so_far += column_rank;
            _line_byte_size += size_of_types[i]*total_rank_size;
        }
        // We also calculate the line size very useful when we start data saving

        total_header_size += sizeof(uint32_t); // 1.
        total_header_size += sizeof(uint16_t); // 2.
        total_header_size += binary_header_size; // 3.
        total_header_size += sizeof(int32_t); // 4.

        // Now we allocate the memory!
        std::size_t buffer_loc = 0;
        auto buffer = std::string(total_header_size, 'A');
        _line_buffer = std::string(_line_byte_size, 'A');

        // Now we fill buffer.
        total_ranks_so_far = 0;
        _copy_number_to_buff(endianess, buffer, buffer_loc); // 1.
        _copy_number_to_buff(binary_header_size, buffer, buffer_loc); // 2.
        for (std::size_t i = 0; i < n_cols; i++) { // 3.
            auto column_name = _names[i];
            auto column_type = parameters_types_str[i];
            auto column_rank = _ranks[i];

            _copy_str_to_buff(column_name, buffer, buffer_loc);
            _copy_str_to_buff(";", buffer, buffer_loc);
            _copy_str_to_buff(column_type, buffer, buffer_loc);
            _copy_str_to_buff(";", buffer, buffer_loc);
            for(std::size_t size_j = 0; size_j < column_rank; size_j++) {
                _copy_str_to_buff(std::to_string(_sizes[total_ranks_so_far + size_j]),
                                  buffer, buffer_loc);

                if(size_j != column_rank - 1) {
                    _copy_str_to_buff(",", buffer, buffer_loc);
                }
            }

            _copy_str_to_buff(";", buffer, buffer_loc);
            total_ranks_so_far += column_rank;
        }

        int32_t j = 0x00000000;
        _copy_number_to_buff(j, buffer, buffer_loc); // 4.

        // save to file.
        _stream << buffer;
    }

    // Save item from tuple in position i
    template<std::size_t i>
    void _save_item(const tuple_type& items, std::size_t& loc) {
        const auto& item = std::get<i>(items);
        using T = decltype(item);

        auto rank = _ranks[i];
        auto total_rank_up_to_i = std::accumulate(_ranks.begin(),
                                                  &_ranks[i],
                                                  0);

        const auto& size_index_start = total_rank_up_to_i;
        const auto& total_size = std::accumulate(&_sizes[size_index_start],
                                                 &_sizes[size_index_start + rank],
                                                 0);

        std::memcpy(&_line_buffer[loc], item, total_size*sizeof(T));
        loc += total_size*sizeof(T);
    }

    // Think of t his function as a wrapper between _save_item
    // and _save_data
    template<std::size_t... I>
    void _save_item_helper(const tuple_type& data, std::index_sequence<I...>) {
        (_save_item<I>(data, _line_buffer_loc),...);
    }

    void _save_event(const tuple_type& data) {
        _line_buffer_loc = 0;
        _save_item_helper(data, std::make_index_sequence<n_cols>{});
        _stream << _line_buffer;
    }

 public:
    DynamicWriter(std::string_view file_name,
                  const std::array<std::string, n_cols>& columns_names,
                  const std::array<std::size_t, n_cols>& columns_ranks,
                  const std::vector<std::size_t>& columns_sizes) :
        _file_name{file_name},
        _names{columns_names},
        _ranks{columns_ranks},
        _sizes{columns_sizes}
    {
        total_ranks = std::accumulate(columns_ranks.begin(),
                                      columns_ranks.end(), 0);

        _stream.open(_file_name,
                     std::ofstream::app | std::ofstream::binary);

        if (_stream.is_open()) {
            _open = true;
            if (std::filesystem::is_empty(file_name)) {
                _create_file();
            }
        }
    }

    ~DynamicWriter() {
        _open = false;
        _stream.close();
    }

    void add(const DataTypes&... data) noexcept {
        _queue.enqueue(std::make_tuple(data...));
    }

    bool save() noexcept {
        auto approx_length = _queue.size_approx();
        auto data = std::vector<tuple_type>(approx_length);
        if (not _queue.try_dequeue_bulk(data.data(), approx_length)) {
            return false;
        }

        for(const auto& event : data) {
            _save_event(event);
        }

        return true;
    }
};

} // namespace BinaryFormat
} // namespace SBCQueens

#endif //SBCBINARYFORMAT_H
