#include "foopodbridge/core/database/comparator.h"

#include <algorithm>

namespace foopodbridge::core::database {

comparison_report comparator::exact_bytes(
    std::span<const std::byte> expected,
    std::span<const std::byte> actual) const {
    comparison_report report{true, {}};
    if (expected.size() != actual.size()) {
        report.equal = false;
        report.differences.push_back("byte length changed");
        return report;
    }
    const auto mismatch = std::mismatch(expected.begin(), expected.end(), actual.begin());
    if (mismatch.first != expected.end()) {
        report.equal = false;
        report.differences.push_back("first unexpected byte change found");
    }
    return report;
}

comparison_report comparator::semantic(
    const database_model& expected,
    const database_model& actual) const {
    comparison_report report{true, {}};
    if (expected.persistent_id != actual.persistent_id) {
        report.differences.push_back("database persistent ID changed");
    }
    if (expected.tracks != actual.tracks) {
        report.differences.push_back("track semantics or order changed");
    }
    if (expected.master_playlist != actual.master_playlist) {
        report.differences.push_back("master playlist semantics changed");
    }
    if (expected.playlists != actual.playlists) {
        report.differences.push_back("playlist semantics or order changed");
    }
    report.equal = report.differences.empty();
    return report;
}

comparison_report comparator::preservation(
    const database_document& original,
    std::span<const std::byte> output) const {
    return exact_bytes(original.original_bytes, output);
}

}  // namespace foopodbridge::core::database
