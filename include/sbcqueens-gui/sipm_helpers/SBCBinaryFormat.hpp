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
#include <type_traits>

// C++ 3rd party includes
// my includes
#include "sbcqueens-gui/file_helpers.hpp"

namespace SBCQueens {
namespace SBCBinaryFormatWriterTools {

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
    return {std::is_arithmetic_v<T>...};
}

template<typename... T>
concept is_arithmetic_unpack = std::all_of(is_arithmetic_unpack_helper<T...>().begin(),
                                           is_arithmetic_unpack_helper<T...>().end(),
                                           [](bool v) { return v; });

template<typename T>
requires std::is_arithmetic_v<T>
constexpr static std::string_view type_to_string() {
    using T_no_const = std::remove_reference_t<std::remove_const_t<T>>;
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

} // SBCBinaryFormatWriterTools

template<typename DataType>
class SBCBinaryFormatWriter : public DataFile<DataType> {
    std::size_t _line_byte_size = 0;

 public:
    SBCBinaryFormatWriter(std::string_view) = delete;

    template<size_t NCols, typename... Args>
    requires SBCBinaryFormatWriterTools::is_arithmetic_unpack<Args...>
    SBCBinaryFormatWriter(std::string_view file_name,
                          const std::array<std::string, NCols>& columns_names,
                          const std::array<std::vector<std::size_t>, NCols>& columns_sizes) :
        DataFile<DataType>(file_name) {
        static_assert(sizeof...(Args) == NCols, "The number of args passed "
                                                "must be equal to the number "
                                                "of columns.");

        /*  Header of a binary format is divided in 4 parts:
            1.- Edianess            - always 4 bits long.
            2.- Data Header size    - always 2 bits long (uint16_t)
                                    and is the length of the next bit of data
            3.- Header              - is data header long. Contains the
                                    structure of each line. It is always as
                                    "{name_col};{type_col};{size1},{size2}...;...;
                                    Cannot be longer than 65536 bytes.
            4.- Number of lines     - always 4 bits long. Number of lines
                                    in the file. If 0, it is indefinitely long.
        */
        // The enum and the map is to simplify the code for the lambdas.
        // and the header could potentially change any moment with any type
        enum class Types { CHAR, INT8, INT16, INT32, INT64, UINT8, UINT16,
            UINT32, UINT64, SINGLE, FLOAT, DOUBLE, FLOAT128 };

        const std::unordered_map<Types, std::string> type_to_string = {
                {Types::CHAR, "char"},
                {Types::INT8, "int8"}, {Types::INT16, "int16"},
                {Types::INT32, "int32"}, {Types::UINT8, "uint8"},
                {Types::UINT16, "uint16"}, {Types::UINT32, "uint32"},
                {Types::SINGLE, "single"}, {Types::FLOAT, "single"},
                {Types::DOUBLE, "double"}, {Types::FLOAT128, "float128"}
        };

        // Edianess first
        uint32_t endianess;
        if(std::endian::native == std::endian::big) {
            endianess = 0x040300201;
        } else if (std::endian::native == std::endian::little) {
            endianess = 0x010200304;
        }

        auto parameters_sizes = { sizeof(Args)... };
        auto parameters_types_str = { SBCBinaryFormatWriterTools::type_to_string<Args>()... };

        std::size_t total_header_size = 0;
        std::size_t binary_header_size = 0;
        for (std::size_t i = 0; i < NCols; i++) {
            auto column_sizes = columns_sizes[i];
            // + 1 for the ; character
            binary_header_size += columns_names[i].length() + 1;
            // + 1 for the ; character
            binary_header_size += parameters_types_str[i].length() + 1;

            if (column_sizes.size() == 1) {
                // +1 for the ; character at the end of the size
                binary_header_size += column_sizes[0] + 1;
            } else {
                for (const std::size_t& size : column_sizes) {
                    binary_header_size += size;
                }

                // ..size() for every , added minus - at the end which
                // is replaced by a ;
                binary_header_size += column_sizes.size();
            }

            // We also calculate the  - very useful when we start data
            _line_byte_size += parameters_sizes[i]*std::accumulate(column_sizes.begin(),
                                                                   column_sizes.end(), 0);
        }

        total_header_size += sizeof(uint32_t);
    }
};

} // namespace SBCQueens

#endif //SBCBINARYFORMAT_H
