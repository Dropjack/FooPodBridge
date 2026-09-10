#include "database_test_support.h"

#include "foopodbridge/core/database/comparator.h"
#include "foopodbridge/core/database/hash58.h"
#include "foopodbridge/core/database/reader.h"
#include "foopodbridge/core/database/validator.h"

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

namespace {

[[nodiscard]] std::vector<std::byte> read_bytes(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("private fixture could not be opened");
    }
    const std::vector<char> chars{
        std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    std::vector<std::byte> bytes;
    bytes.reserve(chars.size());
    for (const auto value : chars) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(value)));
    }
    return bytes;
}

void verify_fixture(const std::filesystem::path& path) {
    using namespace foopodbridge::core::database;
    if (!std::filesystem::is_regular_file(path)) {
        return;
    }
    const auto bytes = read_bytes(path);
    const auto profile = traditional_preserve_only_profile();
    const auto parsed = reader{}.read(bytes, profile, "private-fixture");
    foopodbridge::tests::require(parsed.has_value(), "private fixture Reader validation failed");
    foopodbridge::tests::require(
        validator{}.validate(parsed.value()).has_value(), "private fixture model validation failed");
    const auto output = writer{}.write(parsed.value(), profile);
    foopodbridge::tests::require(output.has_value(), "private fixture preserve-only write failed");
    foopodbridge::tests::require(
        comparator{}.preservation(parsed.value(), output.value()).equal,
        "private fixture no-op bytes changed");
    edit_plan forbidden;
    forbidden.operations.push_back(add_track{"x", "x", "x", "x"});
    const auto rejected = editor{}.apply(parsed.value(), forbidden, foopodbridge::tests::fixed_generation());
    foopodbridge::tests::require(
        !rejected && rejected.error().code == error_code::profile_not_writable,
        "private fixture accepted a semantic edit");
}

[[nodiscard]] std::optional<foopodbridge::core::database::hash58_device_key> read_private_hash58_key(
    const std::filesystem::path& path) {
    using namespace foopodbridge::core::database;
    char* environment_key{};
    std::size_t environment_key_size{};
    if (_dupenv_s(&environment_key, &environment_key_size, "FOOPODBRIDGE_PRIVATE_HASH58_KEY") != 0) {
        throw std::runtime_error("private hash58 environment input could not be read");
    }
    if (environment_key != nullptr) {
        auto parsed = parse_hash58_device_key(environment_key);
        std::fill_n(environment_key, environment_key_size, '\0');
        std::free(environment_key);
        if (!parsed) {
            throw std::runtime_error("private hash58 environment input is invalid");
        }
        return std::move(parsed.value());
    }
    if (!std::filesystem::is_regular_file(path)) {
        return std::nullopt;
    }
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("private hash58 key input could not be opened");
    }
    std::string text;
    std::getline(input, text);
    if (!text.empty() && text.back() == '\r') {
        text.pop_back();
    }
    std::string trailing;
    if (std::getline(input, trailing) && !trailing.empty()) {
        throw std::runtime_error("private hash58 key input contains unexpected extra data");
    }
    auto parsed = parse_hash58_device_key(text);
    if (!parsed) {
        throw std::runtime_error("private hash58 key input is invalid");
    }
    return std::move(parsed.value());
}

void verify_nano4_hash58(
    const std::filesystem::path& path,
    const std::optional<foopodbridge::core::database::hash58_device_key>& device_key) {
    using namespace foopodbridge::core::database;
    if (!std::filesystem::is_regular_file(path)) {
        return;
    }
    const auto bytes = read_bytes(path);
    const auto profile = traditional_hash58_profile();
    const auto structural = reader{}.read(bytes, profile, "private-nano4-hash58-structure");
    foopodbridge::tests::require(
        structural.has_value() && structural.value().hash58_status == hash58_signature_status::not_checked,
        "private Nano 4 hash58 structure validation failed");
    if (!device_key.has_value()) {
        return;
    }
    const auto verified = reader{}.read(bytes, profile, *device_key, "private-nano4-hash58-signature");
    foopodbridge::tests::require(
        verified.has_value() && verified.value().hash58_status == hash58_signature_status::valid,
        "private Nano 4 hash58 signature validation failed");
    foopodbridge::tests::require(
        validator{}.validate(verified.value()).has_value(),
        "private Nano 4 signed model validation failed");
}

}  // namespace

int main() {
    try {
        const std::filesystem::path root{FOOPODBRIDGE_PRIVATE_FIXTURE_ROOT};
        verify_fixture(root / "nano4-20260908-original" / "iTunes" / "iTunesDB");
        verify_fixture(root / "nano4-20260909-restored-clean-windows" / "iTunes" / "iTunesDB");
        verify_fixture(root / "ipod55g-20260909-clean-windows-readonly" / "iTunes" / "iTunesDB");
        const auto device_key = read_private_hash58_key(root / "nano4-hash58-device-key.txt");
        verify_nano4_hash58(
            root / "nano4-20260908-original" / "iTunes" / "iTunesDB", device_key);
        verify_nano4_hash58(
            root / "nano4-20260909-restored-clean-windows" / "iTunes" / "iTunesDB", device_key);
        if (!device_key.has_value()) {
            std::cout << "private Nano 4 hash58 key unavailable; signature cross-check skipped\n";
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
