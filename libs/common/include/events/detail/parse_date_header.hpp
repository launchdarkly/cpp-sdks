#pragma once

#include <chrono>
#include <iomanip>
#include <optional>
#include <sstream>

namespace launchdarkly::events::detail {

template <typename Clock>
static std::optional<typename Clock::time_point> ParseDateHeader(
    std::string const& datetime) {
    // TODO: SC-199582
    std::tm t = {};
    std::istringstream ss(datetime);
    ss.imbue(std::locale("en_US.utf-8"));
    ss >> std::get_time(&t, "%a, %d %b %Y %H:%M:%S GMT");
    if (ss.fail()) {
        return std::nullopt;
    }
    std::time_t tt = std::mktime(&t);
    return Clock::from_time_t(tt);
}

}  // namespace launchdarkly::events::detail
