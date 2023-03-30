#include "config/detail/application_info.hpp"

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <cctype>

namespace launchdarkly::config::detail {

ApplicationInfo::Tag::Tag(std::string key, std::string value)
    : key(std::move(key)), value(std::move(value)) {}

std::string ApplicationInfo::Tag::build() const {
    return key + '/' + value;
}

bool valid_char(char c) {
    return std::isalnum(c) || c == '-' || c == '.' || c == '_';
}

bool is_valid_tag(std::string const& key, std::string const& value) {
    if (value.empty() || key.empty()) {
        return false;
    }
    if (value.length() > 64) {
        return false;
    }
    if (!std::all_of(key.begin(), key.end(), valid_char)) {
        return false;
    }
    return std::all_of(value.begin(), value.end(), valid_char);
}

ApplicationInfo& ApplicationInfo::add_tag(std::string key, std::string value) {
    if (is_valid_tag(key, value)) {
        tags_.emplace_back(std::move(key), std::move(value));
    }
    // todo: error handling if not valid
    return *this;
}
ApplicationInfo& ApplicationInfo::app_identifier(std::string app_id) {
    return add_tag("application-id", std::move(app_id));
}

ApplicationInfo& ApplicationInfo::app_version(std::string version) {
    return add_tag("application-version", std::move(version));
}

std::optional<std::string> ApplicationInfo::build() const {
    if (tags_.empty()) {
        return std::nullopt;
    }

    // Insert all tags into 'pairs' formatted as 'key/value'.
    std::vector<std::string> pairs(tags_.size());
    std::transform(tags_.cbegin(), tags_.cend(), pairs.begin(),
                   [](auto tag) { return tag.build(); });

    // Sort so the result is deterministic.
    std::sort(pairs.begin(), pairs.end());

    // Remove any duplicate tags.
    pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());

    // Concatenate with space as the delimiter.
    return boost::algorithm::join(pairs, " ");
}

}  // namespace launchdarkly::config::detail
