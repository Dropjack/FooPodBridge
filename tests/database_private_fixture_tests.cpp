#include "database_test_support.h"

#include "foopodbridge/core/database/comparator.h"
#include "foopodbridge/core/database/reader.h"
#include "foopodbridge/core/database/validator.h"

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
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

}  // namespace

int main() {
    try {
        const std::filesystem::path root{FOOPODBRIDGE_PRIVATE_FIXTURE_ROOT};
        verify_fixture(root / "nano4-20260908-original" / "iTunes" / "iTunesDB");
        verify_fixture(root / "nano4-20260909-restored-clean-windows" / "iTunes" / "iTunesDB");
        verify_fixture(root / "ipod55g-20260909-clean-windows-readonly" / "iTunes" / "iTunesDB");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
