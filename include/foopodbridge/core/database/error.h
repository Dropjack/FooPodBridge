#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace foopodbridge::core::database {

enum class error_code {
    truncated_input,
    invalid_marker,
    header_too_small,
    section_out_of_bounds,
    length_overflow,
    count_exceeds_section,
    unsupported_container,
    unsupported_encoding,
    resource_limit_exceeded,
    invalid_text,
    duplicate_track_id,
    duplicate_persistent_id,
    missing_master_playlist,
    multiple_master_playlists,
    dangling_track_reference,
    duplicate_playlist_member,
    profile_mismatch,
    profile_not_writable,
    opaque_dependency,
    id_exhausted,
    generated_output_invalid,
    comparison_mismatch,
    invalid_edit,
    missing_device_key,
    invalid_device_key_length,
    invalid_device_key_character,
    hash_field_out_of_bounds,
    invalid_hash_scheme,
    missing_hash58,
    hash58_mismatch,
    unsupported_signed_profile,
};

struct database_error {
    error_code code{};
    std::uint64_t offset{};
    std::string marker;
    std::string path;
    std::string summary;
};

template <typename T>
class result final {
public:
    static result success(T value) { return result(std::move(value)); }
    static result failure(database_error error) { return result(std::move(error)); }

    [[nodiscard]] bool has_value() const noexcept { return std::holds_alternative<T>(storage_); }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }
    [[nodiscard]] T& value() { return std::get<T>(storage_); }
    [[nodiscard]] const T& value() const { return std::get<T>(storage_); }
    [[nodiscard]] database_error& error() { return std::get<database_error>(storage_); }
    [[nodiscard]] const database_error& error() const { return std::get<database_error>(storage_); }

private:
    explicit result(T value) : storage_(std::move(value)) {}
    explicit result(database_error error) : storage_(std::move(error)) {}

    std::variant<T, database_error> storage_;
};

template <>
class result<void> final {
public:
    static result success() { return result(); }
    static result failure(database_error error) { return result(std::move(error)); }

    [[nodiscard]] bool has_value() const noexcept { return !error_.has_value(); }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }
    [[nodiscard]] database_error& error() { return *error_; }
    [[nodiscard]] const database_error& error() const { return *error_; }

private:
    result() = default;
    explicit result(database_error error) : error_(std::move(error)) {}

    std::optional<database_error> error_;
};

}  // namespace foopodbridge::core::database
