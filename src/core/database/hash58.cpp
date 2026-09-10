// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (C) 2007 Christophe Fergeau <teuf@gnome.org>
 *
 * The key-derivation and database-canonicalization algorithm in this file is
 * based on libgpod's itdb_hash58.c. It has been rewritten in C++20 without
 * GLib or other runtime dependencies.
 * FooPodBridge modification: 2026-09-10, isolated strict key parsing,
 * streaming canonicalization, SHA-1/HMAC, verification, and typed errors.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "foopodbridge/core/database/hash58.h"

#include "internal.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <numeric>

namespace foopodbridge::core::database {
namespace {

constexpr std::size_t database_id_offset = 0x18U;
constexpr std::size_t scheme_offset = 0x30U;
constexpr std::size_t prehash_offset = 0x32U;
constexpr std::size_t hash_offset = 0x58U;
constexpr std::size_t digest_size = 20U;
constexpr std::size_t minimum_hash_end = hash_offset + digest_size;

constexpr std::array<std::uint8_t, 256> substitution_table{
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16,
};

constexpr std::array<std::uint8_t, 256> inverse_substitution_table{
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d,
};
constexpr std::array<std::byte, 18> fixed_key_material{
    std::byte{0x67}, std::byte{0x23}, std::byte{0xfe}, std::byte{0x30}, std::byte{0x45}, std::byte{0x33},
    std::byte{0xf8}, std::byte{0x90}, std::byte{0x99}, std::byte{0x21}, std::byte{0x07}, std::byte{0xc1},
    std::byte{0xd0}, std::byte{0x12}, std::byte{0xb2}, std::byte{0xa1}, std::byte{0x07}, std::byte{0x81},
};

static_assert(substitution_table[0x00] == 0x63 && inverse_substitution_table[0x63] == 0x00);
static_assert(substitution_table[0xff] == 0x16 && inverse_substitution_table[0x16] == 0xff);

class sha1 final {
public:
    void update(std::span<const std::byte> bytes) noexcept {
        total_bytes_ += bytes.size();
        for (const auto value : bytes) {
            buffer_[buffer_size_++] = value;
            if (buffer_size_ == buffer_.size()) {
                transform(buffer_);
                buffer_size_ = 0U;
            }
        }
    }

    [[nodiscard]] hash58_digest finish() noexcept {
        const auto bit_length = total_bytes_ * 8U;
        const std::array<std::byte, 1> marker{std::byte{0x80}};
        update(marker);
        const std::array<std::byte, 64> zeros{};
        if (buffer_size_ > 56U) {
            update(std::span<const std::byte>(zeros).first(64U - buffer_size_));
        }
        update(std::span<const std::byte>(zeros).first(56U - buffer_size_));
        std::array<std::byte, 8> length_bytes{};
        for (std::size_t index = 0; index < length_bytes.size(); ++index) {
            length_bytes[index] = static_cast<std::byte>(bit_length >> ((7U - index) * 8U));
        }
        update(length_bytes);

        hash58_digest digest{};
        for (std::size_t word = 0; word < state_.size(); ++word) {
            for (std::size_t byte = 0; byte < 4U; ++byte) {
                digest[word * 4U + byte] = static_cast<std::byte>(state_[word] >> ((3U - byte) * 8U));
            }
        }
        return digest;
    }

private:
    void transform(const std::array<std::byte, 64>& block) noexcept {
        std::array<std::uint32_t, 80> words{};
        for (std::size_t index = 0; index < 16U; ++index) {
            const auto offset = index * 4U;
            words[index] = (static_cast<std::uint32_t>(block[offset]) << 24U) |
                (static_cast<std::uint32_t>(block[offset + 1U]) << 16U) |
                (static_cast<std::uint32_t>(block[offset + 2U]) << 8U) |
                static_cast<std::uint32_t>(block[offset + 3U]);
        }
        for (std::size_t index = 16U; index < words.size(); ++index) {
            words[index] = std::rotl(
                words[index - 3U] ^ words[index - 8U] ^ words[index - 14U] ^ words[index - 16U], 1);
        }

        auto a = state_[0];
        auto b = state_[1];
        auto c = state_[2];
        auto d = state_[3];
        auto e = state_[4];
        for (std::size_t index = 0; index < words.size(); ++index) {
            std::uint32_t function{};
            std::uint32_t constant{};
            if (index < 20U) {
                function = (b & c) | ((~b) & d);
                constant = 0x5a827999U;
            } else if (index < 40U) {
                function = b ^ c ^ d;
                constant = 0x6ed9eba1U;
            } else if (index < 60U) {
                function = (b & c) | (b & d) | (c & d);
                constant = 0x8f1bbcdcU;
            } else {
                function = b ^ c ^ d;
                constant = 0xca62c1d6U;
            }
            const auto temporary = std::rotl(a, 5) + function + e + constant + words[index];
            e = d;
            d = c;
            c = std::rotl(b, 30);
            b = a;
            a = temporary;
        }
        state_[0] += a;
        state_[1] += b;
        state_[2] += c;
        state_[3] += d;
        state_[4] += e;
    }

    std::array<std::uint32_t, 5> state_{0x67452301U, 0xefcdab89U, 0x98badcfeU, 0x10325476U, 0xc3d2e1f0U};
    std::array<std::byte, 64> buffer_{};
    std::size_t buffer_size_{};
    std::uint64_t total_bytes_{};
};

[[nodiscard]] database_error hash_error(error_code code, std::string summary) {
    return detail::make_error(code, hash_offset, "mhbd", "hash58", std::move(summary));
}

[[nodiscard]] result<void> validate_envelope(std::span<const std::byte> bytes) {
    if (bytes.size() < minimum_hash_end) {
        return result<void>::failure(hash_error(
            error_code::hash_field_out_of_bounds, "database is too small to contain the hash58 field"));
    }
    if (!detail::marker_is(detail::read_marker(bytes, 0U), "mhbd")) {
        return result<void>::failure(detail::make_error(
            error_code::invalid_marker, 0U, detail::marker_string(detail::read_marker(bytes, 0U)),
            "hash58/root", "hash58 input does not begin with an mhbd record"));
    }
    const auto header_size = static_cast<std::size_t>(detail::read_u32(bytes, 4U));
    if (header_size < minimum_hash_end || header_size > bytes.size()) {
        return result<void>::failure(hash_error(
            error_code::hash_field_out_of_bounds, "mhbd header does not contain the complete hash58 field"));
    }
    return result<void>::success();
}

void update_canonical_database(sha1& digest, std::span<const std::byte> bytes) noexcept {
    const std::array<std::byte, 20> zeros{};
    const std::array<std::byte, 2> scheme{std::byte{1}, std::byte{0}};
    digest.update(bytes.first(database_id_offset));
    digest.update(std::span<const std::byte>(zeros).first(8U));
    digest.update(bytes.subspan(database_id_offset + 8U, scheme_offset - (database_id_offset + 8U)));
    digest.update(scheme);
    digest.update(zeros);
    digest.update(bytes.subspan(prehash_offset + digest_size, hash_offset - (prehash_offset + digest_size)));
    digest.update(zeros);
    digest.update(bytes.subspan(minimum_hash_end));
}

[[nodiscard]] hash58_digest hmac_database(
    const hash58_derived_key& key,
    std::span<const std::byte> bytes) noexcept {
    std::array<std::byte, 64> inner_pad{};
    std::array<std::byte, 64> outer_pad{};
    std::fill(inner_pad.begin(), inner_pad.end(), std::byte{0x36});
    std::fill(outer_pad.begin(), outer_pad.end(), std::byte{0x5c});
    for (std::size_t index = 0; index < key.size(); ++index) {
        inner_pad[index] ^= key[index];
        outer_pad[index] ^= key[index];
    }
    sha1 inner;
    inner.update(inner_pad);
    update_canonical_database(inner, bytes);
    const auto inner_digest = inner.finish();
    sha1 outer;
    outer.update(outer_pad);
    outer.update(inner_digest);
    return outer.finish();
}

[[nodiscard]] int hex_value(char value) noexcept {
    if (value >= '0' && value <= '9') {
        return value - '0';
    }
    if (value >= 'a' && value <= 'f') {
        return value - 'a' + 10;
    }
    if (value >= 'A' && value <= 'F') {
        return value - 'A' + 10;
    }
    return -1;
}

}  // namespace

result<hash58_device_key> parse_hash58_device_key(std::string_view text) {
    if (text.empty()) {
        return result<hash58_device_key>::failure(hash_error(
            error_code::missing_device_key, "hash58 device key is required"));
    }
    if (text.size() != 16U) {
        return result<hash58_device_key>::failure(hash_error(
            error_code::invalid_device_key_length, "hash58 device key must contain exactly 16 hexadecimal characters"));
    }
    std::array<std::byte, 8> bytes{};
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        const auto high = hex_value(text[index * 2U]);
        const auto low = hex_value(text[index * 2U + 1U]);
        if (high < 0 || low < 0) {
            return result<hash58_device_key>::failure(hash_error(
                error_code::invalid_device_key_character, "hash58 device key contains a non-hexadecimal character"));
        }
        bytes[index] = static_cast<std::byte>((high << 4) | low);
    }
    return result<hash58_device_key>::success(hash58_device_key{bytes});
}

hash58_derived_key derive_hash58_key(const hash58_device_key& device_key) noexcept {
    std::array<std::byte, 16> mapped{};
    for (std::size_t pair = 0; pair < 4U; ++pair) {
        const auto first = static_cast<unsigned>(device_key.bytes_[pair * 2U]);
        const auto second = static_cast<unsigned>(device_key.bytes_[pair * 2U + 1U]);
        const auto common_multiple = first == 0U || second == 0U
            ? 1U
            : (first / std::gcd(first, second)) * second;
        const auto high = static_cast<std::uint8_t>((common_multiple >> 8U) & 0xffU);
        const auto low = static_cast<std::uint8_t>(common_multiple & 0xffU);
        mapped[pair * 4U] = static_cast<std::byte>(substitution_table[high]);
        mapped[pair * 4U + 1U] = static_cast<std::byte>(inverse_substitution_table[high]);
        mapped[pair * 4U + 2U] = static_cast<std::byte>(substitution_table[low]);
        mapped[pair * 4U + 3U] = static_cast<std::byte>(inverse_substitution_table[low]);
    }
    sha1 digest;
    digest.update(fixed_key_material);
    digest.update(mapped);
    return digest.finish();
}

result<hash58_digest> compute_hash58(
    const hash58_device_key& device_key,
    std::span<const std::byte> database_bytes) {
    const auto envelope = validate_envelope(database_bytes);
    if (!envelope) {
        return result<hash58_digest>::failure(envelope.error());
    }
    return result<hash58_digest>::success(hmac_database(derive_hash58_key(device_key), database_bytes));
}

result<void> verify_hash58(
    const hash58_device_key& device_key,
    std::span<const std::byte> database_bytes) {
    const auto envelope = validate_envelope(database_bytes);
    if (!envelope) {
        return envelope;
    }
    if (database_bytes[scheme_offset] != std::byte{1} || database_bytes[scheme_offset + 1U] != std::byte{0}) {
        return result<void>::failure(hash_error(
            error_code::invalid_hash_scheme, "database does not declare hash58 scheme 1"));
    }
    const auto stored = database_bytes.subspan(hash_offset, digest_size);
    if (std::all_of(stored.begin(), stored.end(), [](std::byte value) { return value == std::byte{0}; })) {
        return result<void>::failure(hash_error(error_code::missing_hash58, "database hash58 field is empty"));
    }
    const auto computed = hmac_database(derive_hash58_key(device_key), database_bytes);
    std::uint8_t difference{};
    for (std::size_t index = 0; index < computed.size(); ++index) {
        difference |= static_cast<std::uint8_t>(computed[index] ^ stored[index]);
    }
    if (difference != 0U) {
        return result<void>::failure(hash_error(error_code::hash58_mismatch, "database hash58 verification failed"));
    }
    return result<void>::success();
}

}  // namespace foopodbridge::core::database
