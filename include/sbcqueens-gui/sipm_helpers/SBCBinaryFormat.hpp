//
// Created by Hector Hawley Herrera on 2023-02-19.
//

#ifndef SBCBINARYFORMAT_H
#define SBCBINARYFORMAT_H
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
// my includes
#include "sbcqueens-gui/file_helpers.hpp"

namespace SBCQueens {
namespace BinaryFormat {
namespace Tools {
    template<typename T>
    constexpr static std::size_t get_size() {
        return sizeof(T);
    }

    template<std::size_t N>
    constexpr static std::size_t get_size() {
        return N;
    }

    template<typename... T>
    constexpr static auto is_arithmetic_unpack_helper() -> std::array<bool, sizeof...(T)> {
        return {std::is_arithmetic_v<
                std::remove_cvref_t<std::remove_pointer<
                        std::remove_all_extents_t<T>>>>...};
    }

    template<typename... T>
    concept is_arithmetic_unpack = std::all_of(is_arithmetic_unpack_helper<T...>().begin(),
                                               is_arithmetic_unpack_helper<T...>().end(),
                                               [](bool v) { return v; });

    // For this API, 1-D arrays have the same rank as constants
    template<typename T>
    constexpr static std::size_t rank_constants() {
        if constexpr (std::rank_v<T> == 0) {
            return 1;
        } else {
            return std::rank_v<T>;
        }
    }

    template<typename T>
    requires std::is_arithmetic_v<T>
    constexpr static std::string_view type_to_string() {
        // We get the most pure essence of T: no consts, no references,
        // and no []
        using T_no_const = std::remove_all_extents_t<
                std::remove_reference_t<std::remove_const_t<T>>>;
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

template <typename T>
requires std::is_arithmetic_v<std::remove_all_extents_t<T>>
struct BinaryType {
    using type = std::remove_all_extents_t<T>;
    const static std::size_t rank = std::rank_v<T>;
    const std::array<std::size_t, rank> sizes;
};

template<typename... DataTypes>
requires Tools::is_arithmetic_unpack<DataTypes...>
struct DynamicWriter {
    const static std::size_t n_cols = sizeof...(DataTypes);
    const static std::array<std::size_t, n_cols> ranks = { Tools::rank_constants<DataTypes>...};
    const static std::size_t total_ranks = std::accumulate(ranks.begin(),
                                                           ranks.end(),
                                                           0);
private:
    const std::array<std::string, n_cols>& _names;
    const std::array<std::size_t, total_ranks>& _sizes;

    bool _open = false;
    std::string _abs_dir;
    std::ofstream _stream;

    std::size_t _line_byte_size = 0;
    std::size_t _line_buffer_loc = 0;
    std::string _line_buffer;

    template<typename T>
    void _copy_number_to_buff(const T& num,
                              std::string& buffer,
                              std::size_t& loc,
                              const std::size_t& size = sizeof(T)) {
        const char* tmpstr = reinterpret_cast<const char*>(&num);
        for(std::size_t i = loc; i < size + loc; i++) {
            buffer[i] = tmpstr[i - loc];
        }

        loc += size;
    };

    void _copy_str_to_buff(std::string_view source,
                           std::string& buffer,
                           std::size_t& loc) {
        source.copy(&buffer[loc], source.length(), 0);
        loc += source.length();
    }

    void _create_file(const std::array<std::string, n_cols>& columns_names,
                      const std::array<std::size_t, total_ranks>& columns_sizes) {

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
        uint32_t endianess;
        if constexpr (std::endian::native == std::endian::big) {
            endianess = 0x040300201;
        } else if constexpr (std::endian::native == std::endian::little) {
            endianess = 0x010200304;
        }

        // This turns the types into strings, magic!
        constexpr auto parameters_types_str = { Tools::type_to_string<DataTypes>()... };
        constexpr auto size_of_types = { sizeof(DataTypes)... };

        std::size_t total_header_size = 0;
        // has to be uint16_t because we are saving it to the file later
        uint16_t binary_header_size = 0;
        std::size_t total_ranks = 0;
        for (std::size_t i = 0; i < n_cols; i++) {
            auto column_rank = ranks[i];

            // + 1 for the ; character
            binary_header_size += columns_names[i].length() + 1;
            // + 1 for the ; character
            binary_header_size += parameters_types_str[i].length() + 1;

            std::size_t total_rank_size = 0;
            for(std::size_t j = 0; j < column_rank; j ++) {
                auto size = columns_sizes[total_ranks + j];
                total_rank_size += size;
                // It is always + 1 because there is either a ',' or a ';'
                binary_header_size += std::to_string(size).length() + 1;
            }

            total_ranks += column_rank;
            _line_byte_size = size_of_types[i]*total_rank_size;
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
        _copy_number_to_buff(endianess, buffer, buffer_loc); // 1.
        _copy_number_to_buff(binary_header_size, buffer, buffer_loc); // 2.
        for (std::size_t i = 0; i < n_cols; i++) { // 3.
            auto column_name = columns_names[i];
            auto column_type = parameters_types_str[i];
            auto column_sizes = columns_sizes[i];

            _copy_str_to_buff(column_name, buffer, buffer_loc);
            _copy_str_to_buff(";", buffer, buffer_loc);
            _copy_str_to_buff(column_type, buffer, buffer_loc);
            _copy_str_to_buff(";", buffer, buffer_loc);
            for(std::size_t size_j = 0; size_j < column_sizes.size(); size_j++) {
                _copy_str_to_buff(std::to_string(column_sizes[size_j]),
                                  buffer, buffer_loc);

                if(size_j != column_sizes.size() - 1) {
                    _copy_str_to_buff(",", buffer, buffer_loc);
                }
            }

            _copy_str_to_buff(";", buffer, buffer_loc);
        }
        int32_t j = 0x00000000;
        _copy_number_to_buff(j, buffer, buffer_loc); // 4.

        // save to file.
        this->_stream << buffer;
    }

    template<typename T>
    void _save_item(const T& item, const std::size_t& i, std::size_t& loc) {
        constexpr auto rank = ranks[i];
        constexpr auto total_rank_up_to_i = std::accumulate(ranks.begin(),
                                                            &ranks[i],
                                                            0);
        const auto& size_index_start = i + total_rank_up_to_i;
        const auto& total_size = std::accumulate(&_sizes[size_index_start],
                                                 &_sizes[size_index_start + rank],
                                                 0);

        if constexpr (std::is_pointer_v<T>) {
            std::memcpy(&_line_buffer[loc], item, total_size*sizeof(T));
            loc += total_size*sizeof(T);
        } else {
            // if it is an int or int[] or related. Everything we need is
            // already provided for us
            _copy_number_to_buff(item, _line_buffer, loc);
        }
    }

    void _save_data(const DataTypes&... data) {
        _line_buffer_loc = 0;
        (_save_item(data, i, _line_buffer_loc),...);
    }
public:

    DynamicWriter(std::string_view file_name,
                  const std::array<std::string, n_cols>& columns_names,
                  const std::array<size_t, total_ranks>& columns_sizes) :
            _names{columns_names},
            _sizes{columns_sizes} {

        _stream.open(_abs_dir,
                     std::ofstream::app | std::ofstream::binary);

        if (_stream.is_open()) {
            _open = true;
            if (std::filesystem::is_empty(file_name)) {
                _create_file(columns_names, columns_sizes);
            }
        }
    }

    ~DynamicWriter() {
        _open = false;
        _stream.close();
    }


    void save() noexcept {

    }
};

} // namespace BinaryFormat
} // namespace SBCQueens

#endif //SBCBINARYFORMAT_H
