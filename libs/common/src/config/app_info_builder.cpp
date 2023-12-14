#include <launchdarkly/config/shared/builders/app_info_builder.hpp>

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <cctype>

namespace launchdarkly::config::shared::builders {
// Defines the maximum character length for an Application Tag value.
constexpr std::size_t kMaxTagValueLength = 64;

AppInfoBuilder::Tag::Tag(std::string key, std::string value)
    : key(std::move(key)), value(std::move(value)) {
}

tl::expected<std::string, Error> AppInfoBuilder::Tag::Build() const {
    if (auto err = IsValidTag(key, value)) {
        return tl::unexpected(*err);
    }
    return key + '/' + value;
}

bool ValidChar(char c) {
    if (c > 0) {
        // The MSVC implementation of isalnum will assert if the number is
        // outside its lookup table (0-0xFF, inclusive.)
        // iswalnum would not, but is less restrictive than desired.
        return std::isalnum(c) != 0 || c == '-' || c == '.' || c == '_';
    }
    return false;
}

std::optional<Error> IsValidTag(std::string const& key,
                                std::string const& value) {
    if (value.empty() || key.empty()) {
        return ErrorCode::kConfig_ApplicationInfo_EmptyKeyOrValue;
    }
    if (value.length() > kMaxTagValueLength) {
        return ErrorCode::kConfig_ApplicationInfo_ValueTooLong;
    }
    if (!std::all_of(key.begin(), key.end(), ValidChar)) {
        return ErrorCode::kConfig_ApplicationInfo_InvalidKeyCharacters;
    }
    if (!std::all_of(value.begin(), value.end(), ValidChar)) {
        return ErrorCode::kConfig_ApplicationInfo_InvalidValueCharacters;
    }
    return std::nullopt;
}

AppInfoBuilder& AppInfoBuilder::AddTag(std::string key, std::string value) {
    tags_.emplace_back(std::move(key), std::move(value));
    return *this;
}

AppInfoBuilder& AppInfoBuilder::Identifier(std::string app_id) {
    return AddTag("application-id", std::move(app_id));
}

AppInfoBuilder& AppInfoBuilder::Version(std::string version) {
    return AddTag("application-version", std::move(version));
}

std::optional<std::string> AppInfoBuilder::Build() const {
    if (tags_.empty()) {
        return std::nullopt;
    }

    // Build all the tags, which results in pairs of (tag key, value | error).
    std::vector<std::pair<std::string, tl::expected<std::string, Error>>>
        unvalidated(tags_.size());

    std::transform(
        tags_.cbegin(), tags_.cend(), unvalidated.begin(),
        [](auto tag) { return std::make_pair(tag.key, tag.Build()); });

    std::vector<std::string> validated;

    for (auto const& tag : unvalidated) {
        if (!tag.second) {
            // TODO: Figure out how to report. sc-204388
        } else {
            validated.push_back(tag.second.value());
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
} // namespace launchdarkly::config::shared::builders
