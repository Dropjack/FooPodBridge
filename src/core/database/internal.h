#pragma once

#include "foopodbridge/core/database/error.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace foopodbridge::core::database::detail {

[[nodiscard]] inline database_error make_error(
    error_code code,
    std::uint64_t offset,
    std::string marker,
    std::string path,
    std::string summary) {
    return database_error{code, offset, std::move(marker), std::move(path), std::move(summary)};
}

[[nodiscard]] inline bool range_fits(std::size_t offset, std::size_t length, std::size_t limit) noexcept {
    return offset <= limit && length <= limit - offset;
}

[[nodiscard]] inline std::uint32_t read_u32(std::span<const std::byte> bytes, std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(bytes[offset]) |
        (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U) |
        (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U) |
        (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] inline std::uint64_t read_u64(std::span<const std::byte> bytes, std::size_t offset) noexcept {
    const auto low = static_cast<std::uint64_t>(read_u32(bytes, offset));
    const auto high = static_cast<std::uint64_t>(read_u32(bytes, offset + 4U));
    return low | (high << 32U);
}

[[nodiscard]] inline std::array<char, 4> read_marker(std::span<const std::byte> bytes, std::size_t offset) noexcept {
    return {
        static_cast<char>(bytes[offset]),
        static_cast<char>(bytes[offset + 1U]),
        static_cast<char>(bytes[offset + 2U]),
        static_cast<char>(bytes[offset + 3U]),
    };
}

[[nodiscard]] inline std::string marker_string(const std::array<char, 4>& marker) {
    return std::string(marker.data(), marker.size());
}

[[nodiscard]] inline bool marker_is(const std::array<char, 4>& marker, std::string_view expected) noexcept {
    return expected.size() == marker.size() &&
        marker[0] == expected[0] && marker[1] == expected[1] &&
        marker[2] == expected[2] && marker[3] == expected[3];
}

inline void append_u32(std::vector<std::byte>& bytes, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32U; shift += 8U) {
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffU));
    }
}

inline void append_u64(std::vector<std::byte>& bytes, std::uint64_t value) {
    append_u32(bytes, static_cast<std::uint32_t>(value & 0xffffffffULL));
    append_u32(bytes, static_cast<std::uint32_t>(value >> 32U));
}

inline void set_u32(std::vector<std::byte>& bytes, std::size_t offset, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32U; shift += 8U) {
        bytes[offset + shift / 8U] = static_cast<std::byte>((value >> shift) & 0xffU);
    }
}

inline void set_u64(std::vector<std::byte>& bytes, std::size_t offset, std::uint64_t value) {
    set_u32(bytes, offset, static_cast<std::uint32_t>(value & 0xffffffffULL));
    set_u32(bytes, offset + 4U, static_cast<std::uint32_t>(value >> 32U));
}

inline void append_marker(std::vector<std::byte>& bytes, std::string_view marker) {
    for (const char value : marker) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(value)));
    }
}

[[nodiscard]] inline result<std::string> utf16le_to_utf8(
    std::span<const std::byte> bytes,
    std::uint64_t offset,
    std::string path) {
    if ((bytes.size() % 2U) != 0U) {
        return result<std::string>::failure(make_error(
            error_code::invalid_text, offset, "mhod", std::move(path), "known string has an odd byte length"));
    }

    std::string output;
    output.reserve(bytes.size());
    for (std::size_t index = 0; index < bytes.size(); index += 2U) {
        const auto first = static_cast<std::uint16_t>(static_cast<std::uint8_t>(bytes[index])) |
            static_cast<std::uint16_t>(static_cast<std::uint8_t>(bytes[index + 1U]) << 8U);
        std::uint32_t code_point = first;
        if (first >= 0xd800U && first <= 0xdbffU) {
            if (index + 3U >= bytes.size()) {
                return result<std::string>::failure(make_error(
                    error_code::invalid_text, offset + index, "mhod", std::move(path), "truncated UTF-16 surrogate pair"));
            }
            const auto second = static_cast<std::uint16_t>(static_cast<std::uint8_t>(bytes[index + 2U])) |
                static_cast<std::uint16_t>(static_cast<std::uint8_t>(bytes[index + 3U]) << 8U);
            if (second < 0xdc00U || second > 0xdfffU) {
                return result<std::string>::failure(make_error(
                    error_code::invalid_text, offset + index, "mhod", std::move(path), "invalid UTF-16 surrogate pair"));
            }
            code_point = 0x10000U + ((first - 0xd800U) << 10U) + (second - 0xdc00U);
            index += 2U;
        } else if (first >= 0xdc00U && first <= 0xdfffU) {
            return result<std::string>::failure(make_error(
                error_code::invalid_text, offset + index, "mhod", std::move(path), "unexpected UTF-16 low surrogate"));
        }

        if (code_point <= 0x7fU) {
            output.push_back(static_cast<char>(code_point));
        } else if (code_point <= 0x7ffU) {
            output.push_back(static_cast<char>(0xc0U | (code_point >> 6U)));
            output.push_back(static_cast<char>(0x80U | (code_point & 0x3fU)));
        } else if (code_point <= 0xffffU) {
            output.push_back(static_cast<char>(0xe0U | (code_point >> 12U)));
            output.push_back(static_cast<char>(0x80U | ((code_point >> 6U) & 0x3fU)));
            output.push_back(static_cast<char>(0x80U | (code_point & 0x3fU)));
        } else {
            output.push_back(static_cast<char>(0xf0U | (code_point >> 18U)));
            output.push_back(static_cast<char>(0x80U | ((code_point >> 12U) & 0x3fU)));
            output.push_back(static_cast<char>(0x80U | ((code_point >> 6U) & 0x3fU)));
            output.push_back(static_cast<char>(0x80U | (code_point & 0x3fU)));
        }
    }
    return result<std::string>::success(std::move(output));
}

[[nodiscard]] inline result<std::vector<std::uint16_t>> utf8_to_utf16(
    std::string_view text,
    std::string path) {
    std::vector<std::uint16_t> output;
    for (std::size_t index = 0; index < text.size();) {
        const auto first = static_cast<std::uint8_t>(text[index]);
        std::uint32_t code_point{};
        std::size_t length{};
        if (first <= 0x7fU) {
            code_point = first;
            length = 1U;
        } else if ((first & 0xe0U) == 0xc0U) {
            code_point = first & 0x1fU;
            length = 2U;
        } else if ((first & 0xf0U) == 0xe0U) {
            code_point = first & 0x0fU;
            length = 3U;
        } else if ((first & 0xf8U) == 0xf0U) {
            code_point = first & 0x07U;
            length = 4U;
        } else {
            return result<std::vector<std::uint16_t>>::failure(make_error(
                error_code::invalid_text, index, {}, std::move(path), "invalid UTF-8 leading byte"));
        }
        if (!range_fits(index, length, text.size())) {
            return result<std::vector<std::uint16_t>>::failure(make_error(
                error_code::invalid_text, index, {}, std::move(path), "truncated UTF-8 sequence"));
        }
        for (std::size_t continuation = 1U; continuation < length; ++continuation) {
            const auto value = static_cast<std::uint8_t>(text[index + continuation]);
            if ((value & 0xc0U) != 0x80U) {
                return result<std::vector<std::uint16_t>>::failure(make_error(
                    error_code::invalid_text, index, {}, std::move(path), "invalid UTF-8 continuation byte"));
            }
            code_point = (code_point << 6U) | (value & 0x3fU);
        }
        const bool overlong = (length == 2U && code_point < 0x80U) ||
            (length == 3U && code_point < 0x800U) || (length == 4U && code_point < 0x10000U);
        if (overlong || code_point > 0x10ffffU || (code_point >= 0xd800U && code_point <= 0xdfffU)) {
            return result<std::vector<std::uint16_t>>::failure(make_error(
                error_code::invalid_text, index, {}, std::move(path), "invalid UTF-8 code point"));
        }
        if (code_point <= 0xffffU) {
            output.push_back(static_cast<std::uint16_t>(code_point));
        } else {
            code_point -= 0x10000U;
            output.push_back(static_cast<std::uint16_t>(0xd800U + (code_point >> 10U)));
            output.push_back(static_cast<std::uint16_t>(0xdc00U + (code_point & 0x3ffU)));
        }
        index += length;
    }
    return result<std::vector<std::uint16_t>>::success(std::move(output));
}

}  // namespace foopodbridge::core::database::detail
