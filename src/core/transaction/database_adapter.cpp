#include "foopodbridge/core/transaction/database_adapter.h"
#include <algorithm>

namespace foopodbridge::core::transaction {
database_validator traditional_database_validator(database::format_profile profile,
                                                   std::optional<database::hash58_device_key> key) {
    if (profile.kind == database::profile_kind::traditional_preserve_only ||
        (profile.kind == database::profile_kind::traditional_hash58 && !key)) throw failure("profile_read_only");
    return [profile = std::move(profile), key](std::span<const std::byte> data) {
        const auto parsed = key ? database::reader{}.read(data, profile, *key)
                                : database::reader{}.read(data, profile);
        if (!parsed || !database::validator{}.validate(parsed.value())) throw failure("database_invalid");
        std::vector<std::string> references;
        for (const auto& track : parsed.value().model.tracks) {
            auto location = track.location;
            if (location.empty() || location.front() != ':') throw failure("database_path_invalid");
            location.erase(location.begin());
            std::replace(location.begin(), location.end(), ':', '/');
            if (!safe_path(location) || location.rfind("iPod_Control/Music/", 0) != 0) throw failure("database_path_invalid");
            references.push_back(std::move(location));
        }
        return references;
    };
}
}
