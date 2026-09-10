#include "database_test_support.h"

#include "foopodbridge/core/database/comparator.h"
#include "foopodbridge/core/database/hash58.h"
#include "foopodbridge/core/database/reader.h"
#include "foopodbridge/core/database/validator.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void set_u32(std::vector<std::byte>& bytes, std::size_t offset, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32U; shift += 8U) {
        bytes[offset + shift / 8U] = static_cast<std::byte>((value >> shift) & 0xffU);
    }
}

[[nodiscard]] std::uint32_t read_u32(const std::vector<std::byte>& bytes, std::size_t offset) {
    return static_cast<std::uint32_t>(bytes[offset]) |
        (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U) |
        (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U) |
        (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U);
}

template <std::size_t Size>
[[nodiscard]] std::array<std::byte, Size> hex_bytes(std::string_view text) {
    if (text.size() != Size * 2U) {
        throw std::runtime_error("invalid test vector length");
    }
    const auto nibble = [](char value) -> unsigned {
        if (value >= '0' && value <= '9') {
            return static_cast<unsigned>(value - '0');
        }
        if (value >= 'a' && value <= 'f') {
            return static_cast<unsigned>(value - 'a' + 10);
        }
        throw std::runtime_error("invalid test vector character");
    };
    std::array<std::byte, Size> output{};
    for (std::size_t index = 0; index < output.size(); ++index) {
        output[index] = static_cast<std::byte>((nibble(text[index * 2U]) << 4U) | nibble(text[index * 2U + 1U]));
    }
    return output;
}

[[nodiscard]] std::vector<std::byte> make_hash_vector_database() {
    std::vector<std::byte> bytes(512U);
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] = static_cast<std::byte>((index * 37U + 11U) & 0xffU);
    }
    bytes[0] = std::byte{'m'};
    bytes[1] = std::byte{'h'};
    bytes[2] = std::byte{'b'};
    bytes[3] = std::byte{'d'};
    set_u32(bytes, 4U, 244U);
    set_u32(bytes, 8U, static_cast<std::uint32_t>(bytes.size()));
    return bytes;
}

}  // namespace

int main() {
    using namespace foopodbridge::core::database;
    try {
        const auto missing = parse_hash58_device_key("");
        foopodbridge::tests::require(
            !missing && missing.error().code == error_code::missing_device_key,
            "empty device key was not rejected");
        for (std::size_t length = 1U; length <= 15U; ++length) {
            const auto invalid = std::string(length, '0');
            const auto parsed = parse_hash58_device_key(invalid);
            foopodbridge::tests::require(
                !parsed && parsed.error().code == error_code::invalid_device_key_length,
                "short device key was not rejected");
        }
        for (std::size_t length = 17U; length <= 32U; ++length) {
            const auto invalid = std::string(length, '0');
            const auto parsed = parse_hash58_device_key(invalid);
            foopodbridge::tests::require(
                !parsed && parsed.error().code == error_code::invalid_device_key_length,
                "long device key was not rejected");
        }
        const auto non_hex = parse_hash58_device_key("00112233445566g7");
        foopodbridge::tests::require(
            !non_hex && non_hex.error().code == error_code::invalid_device_key_character,
            "non-hexadecimal device key was not rejected");
        for (const auto decorated : {"0x11223344556677", " 011223344556677"}) {
            const auto parsed = parse_hash58_device_key(decorated);
            foopodbridge::tests::require(
                !parsed && parsed.error().code == error_code::invalid_device_key_character,
                "prefixed or whitespace device key was not rejected");
        }

        const auto key = parse_hash58_device_key("0011223344556677");
        const auto uppercase_key = parse_hash58_device_key("00112233445566AA");
        const auto lowercase_key = parse_hash58_device_key("00112233445566aa");
        foopodbridge::tests::require(key.has_value() && uppercase_key.has_value() && lowercase_key.has_value(),
            "valid device key parsing failed");
        foopodbridge::tests::require(
            derive_hash58_key(uppercase_key.value()) == derive_hash58_key(lowercase_key.value()),
            "device key parsing was not case-insensitive");
        foopodbridge::tests::require(
            derive_hash58_key(key.value()) == hex_bytes<20>("4a7e42df48fc6aecbe76622fcd2534f59fd34cb1"),
            "hash58 key derivation golden vector changed");

        auto vector_database = make_hash_vector_database();
        const auto vector_hash = compute_hash58(key.value(), vector_database);
        foopodbridge::tests::require(vector_hash.has_value(), "hash58 golden vector computation failed");
        foopodbridge::tests::require(
            vector_hash.value() == hex_bytes<20>("c0e5d75d2a03855616b9c3d19c935bc81a55be2d"),
            "hash58 HMAC golden vector changed");
        auto alternate_normalized_fields = vector_database;
        std::fill(alternate_normalized_fields.begin() + 0x18U,
            alternate_normalized_fields.begin() + 0x20U, std::byte{0xa5});
        std::fill(alternate_normalized_fields.begin() + 0x32U,
            alternate_normalized_fields.begin() + 0x46U, std::byte{0x5a});
        std::fill(alternate_normalized_fields.begin() + 0x58U,
            alternate_normalized_fields.begin() + 0x6cU, std::byte{0x3c});
        alternate_normalized_fields[0x30U] = std::byte{0xff};
        alternate_normalized_fields[0x31U] = std::byte{0xff};
        const auto normalized_hash = compute_hash58(key.value(), alternate_normalized_fields);
        foopodbridge::tests::require(
            normalized_hash.has_value() && normalized_hash.value() == vector_hash.value(),
            "hash58 canonical fields affected computation");
        vector_database[0x30U] = std::byte{1};
        vector_database[0x31U] = std::byte{0};
        std::copy(vector_hash.value().begin(), vector_hash.value().end(), vector_database.begin() + 0x58U);
        foopodbridge::tests::require(verify_hash58(key.value(), vector_database).has_value(),
            "golden hash58 did not verify");

        auto normalized_fields = vector_database;
        normalized_fields[0x18U] ^= std::byte{0xff};
        normalized_fields[0x32U] ^= std::byte{0xff};
        foopodbridge::tests::require(verify_hash58(key.value(), normalized_fields).has_value(),
            "database ID or prehash normalization changed the signature");
        auto tampered = vector_database;
        tampered[0x80U] ^= std::byte{1};
        const auto tampered_result = verify_hash58(key.value(), tampered);
        foopodbridge::tests::require(
            !tampered_result && tampered_result.error().code == error_code::hash58_mismatch,
            "protected-byte tampering was not detected");
        auto zero_signature = vector_database;
        std::fill(zero_signature.begin() + 0x58U, zero_signature.begin() + 0x6cU, std::byte{0});
        const auto zero_result = verify_hash58(key.value(), zero_signature);
        foopodbridge::tests::require(
            !zero_result && zero_result.error().code == error_code::missing_hash58,
            "zero hash58 field was not distinguished from a mismatch");
        auto wrong_scheme = vector_database;
        wrong_scheme[0x30U] = std::byte{2};
        const auto scheme_result = verify_hash58(key.value(), wrong_scheme);
        foopodbridge::tests::require(
            !scheme_result && scheme_result.error().code == error_code::invalid_hash_scheme,
            "wrong hash scheme was not rejected");
        const auto truncated = compute_hash58(key.value(), std::span<const std::byte>(vector_database).first(0x6bU));
        foopodbridge::tests::require(
            !truncated && truncated.error().code == error_code::hash_field_out_of_bounds,
            "truncated hash58 envelope was not rejected");

        const auto profile = traditional_hash58_profile();
        const auto generation = foopodbridge::tests::fixed_generation();
        const auto empty_a = writer{}.create_empty("Library", profile, key.value(), generation);
        const auto empty_b = writer{}.create_empty("Library", profile, key.value(), generation);
        foopodbridge::tests::require(empty_a.has_value() && empty_b.has_value(),
            "signed empty library generation failed");
        foopodbridge::tests::require(
            empty_a.value().hash58_status == hash58_signature_status::valid &&
                !empty_a.value().has_opaque_dependency,
            "signed empty library did not reread as a valid editable profile");
        foopodbridge::tests::require(
            comparator{}.exact_bytes(empty_a.value().original_bytes, empty_b.value().original_bytes).equal,
            "signed empty library generation was not deterministic");
        foopodbridge::tests::require(
            read_u32(empty_a.value().original_bytes, 4U) == 244U &&
                read_u32(empty_a.value().original_bytes, 16U) == 115U &&
                read_u32(empty_a.value().original_bytes, 20U) == 4U &&
                empty_a.value().original_bytes.size() == 1472U,
            "signed empty library root profile is wrong");
        std::array<std::uint32_t, 4> dataset_types{};
        std::size_t playlist_record_offset{};
        auto dataset_offset = static_cast<std::size_t>(read_u32(empty_a.value().original_bytes, 4U));
        for (std::size_t index = 0; index < dataset_types.size(); ++index) {
            dataset_types[index] = read_u32(empty_a.value().original_bytes, dataset_offset + 12U);
            if (dataset_types[index] == 2U) {
                playlist_record_offset = dataset_offset + profile.dataset_header_size + profile.list_header_size;
            }
            dataset_offset += read_u32(empty_a.value().original_bytes, dataset_offset + 8U);
        }
        foopodbridge::tests::require(
            dataset_types == std::array<std::uint32_t, 4>{4U, 1U, 3U, 2U},
            "signed empty library dataset order is wrong");
        foopodbridge::tests::require(
            playlist_record_offset != 0U &&
                read_u32(empty_a.value().original_bytes, playlist_record_offset + 4U) == 184U,
            "signed empty library did not use the selected playlist header variant");
        foopodbridge::tests::require(
            std::all_of(empty_a.value().original_bytes.begin() + 0x72U,
                empty_a.value().original_bytes.begin() + 0x72U + 46U,
                [](std::byte value) { return value == std::byte{0}; }) &&
            std::all_of(empty_a.value().original_bytes.begin() + 0xabU,
                empty_a.value().original_bytes.begin() + 0xabU + 57U,
                [](std::byte value) { return value == std::byte{0}; }),
            "hash58-only output populated an excluded signature reserve");
        foopodbridge::tests::require(
            verify_hash58(key.value(), empty_a.value().original_bytes).has_value(),
            "signed empty library failed direct verification");

        const auto unkeyed = reader{}.read(empty_a.value().original_bytes, profile, "public-hash58-unkeyed");
        foopodbridge::tests::require(
            unkeyed.has_value() && unkeyed.value().hash58_status == hash58_signature_status::not_checked &&
                validator{}.validate(unkeyed.value()).has_value(),
            "unkeyed structural hash58 read failed");
        const auto wrong_key = parse_hash58_device_key("ffeeddccbbaa9988");
        foopodbridge::tests::require(wrong_key.has_value(), "second public device key parsing failed");
        const auto rejected = reader{}.read(empty_a.value().original_bytes, profile, wrong_key.value(), "public-hash58-wrong-key");
        foopodbridge::tests::require(
            rejected.has_value() && rejected.value().hash58_status == hash58_signature_status::invalid &&
                !validator{}.validate(rejected.value()),
            "wrong device key was not surfaced by Reader and Validator");
        const auto no_key_generation = writer{}.create_empty("Library", profile, generation);
        foopodbridge::tests::require(
            !no_key_generation && no_key_generation.error().code == error_code::missing_device_key,
            "hash58 generation without a device key was not rejected");
        const auto no_op = writer{}.write(empty_a.value(), profile, key.value());
        foopodbridge::tests::require(
            no_op.has_value() && comparator{}.exact_bytes(no_op.value(), empty_a.value().original_bytes).equal,
            "valid hash58 no-op changed bytes");
        const auto wrong_key_no_op = writer{}.write(empty_a.value(), profile, wrong_key.value());
        foopodbridge::tests::require(
            !wrong_key_no_op && wrong_key_no_op.error().code == error_code::hash58_mismatch,
            "hash58 no-op accepted the wrong device key");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
