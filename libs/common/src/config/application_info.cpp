#include "config/detail/application_info.hpp"

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <cctype>

namespace launchdarkly::config::detail {

// Defines the maximum character length for an Application Tag value.
constexpr std::size_t kMaxTagValueLength = 64;

ApplicationInfo::Tag::Tag(std::string key, std::string value)
    : key(std::move(key)), value(std::move(value)) {}

tl::expected<std::string, Error> ApplicationInfo::Tag::build() const {
    if (auto err = IsValidTag(key, value)) {
        return tl::unexpected(*err);
    }
    return key + '/' + value;
}

bool ValidChar(char c) {
    return std::isalnum(c) != 0 || c == '-' || c == '.' || c == '_';
}

std::optional<Error> IsValidTag(std::string const& key,
                                std::string const& value) {
    if (value.empty() || key.empty()) {
        return Error::kConfig_ApplicationInfo_EmptyKeyOrValue;
    }
    if (value.length() > kMaxTagValueLength) {
        return Error::kConfig_ApplicationInfo_ValueTooLong;
    }
    if (!std::all_of(key.begin(), key.end(), ValidChar)) {
        return Error::kConfig_ApplicationInfo_InvalidKeyCharacters;
    }
    if (!std::all_of(value.begin(), value.end(), ValidChar)) {
        return Error::kConfig_ApplicationInfo_InvalidValueCharacters;
    }
    return std::nullopt;
}

ApplicationInfo& ApplicationInfo::add_tag(std::string key, std::string value) {
    tags_.emplace_back(std::move(key), std::move(value));
    return *this;
}
ApplicationInfo& ApplicationInfo::app_identifier(std::string app_id) {
    return add_tag("application-id", std::move(app_id));
}

ApplicationInfo& ApplicationInfo::app_version(std::string version) {
    return add_tag("application-version", std::move(version));
}

std::optional<std::string> ApplicationInfo::build(Logger& logger) const {
    if (tags_.empty()) {
        LD_LOG(logger, LogLevel::kDebug) << "no application tags configured";
        return std::nullopt;
    }

    // Build all the tags, which results in pairs of (tag key, value | error).
    std::vector<std::pair<std::string, tl::expected<std::string, Error>>>
        unvalidated(tags_.size());

    std::transform(
        tags_.cbegin(), tags_.cend(), unvalidated.begin(),
        [](auto tag) { return std::make_pair(tag.key, tag.build()); });

    std::vector<std::string> validated;

    for (auto const& t : unvalidated) {
        if (!t.second) {
            LD_LOG(logger, LogLevel::kWarn)
                << t.second.error() << " for tag '" << t.first << "'";
        } else {
            validated.push_back(t.second.value());
        }
    }

    if (validated.empty()) {
        return std::nullopt;
    }

    // Sort so the result is deterministic.
    std::sort(validated.begin(), validated.end());

    // Remove any duplicate tags.
    validated.erase(std::unique(validated.begin(), validated.end()),
                    validated.end());

    // Concatenate with space as the delimiter.
    return boost::algorithm::join(validated, " ");
}

}  // namespace launchdarkly::config::detail
