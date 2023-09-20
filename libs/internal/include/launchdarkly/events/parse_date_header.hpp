#pragma once

#include <chrono>
#include <iomanip>
#include <optional>
#include <sstream>

namespace launchdarkly::events {

template <typename Clock>

static std::optional<typename Clock::time_point> ParseDateHeader(
    std::string const& datetime,
    std::locale const& locale) {
    // The following comments may not be entirely accurate.
    // TODO: There must be a better way.

    std::tm gmt_tm = {};

    std::istringstream string_stream(datetime);
    string_stream.imbue(locale);
    string_stream >> std::get_time(&gmt_tm, "%a, %d %b %Y %H:%M:%S GMT");
    if (string_stream.fail()) {
        return std::nullopt;
    }
    // Obtain a time_t. Caveat: mktime will interpret the tm as a local time,
    // but it's not, so we're going to need to cancel that out later.
    std::time_t local_t = std::mktime(&gmt_tm);

    // Convert the fake local time into UTC.
    std::tm* utc_tm = std::gmtime(&local_t);
    utc_tm->tm_isdst = false;

    // Now since the utc_tm has the UTC offset baked in, convert it back
    // into a time_t.
    std::time_t gm_t = std::mktime(utc_tm);

    // Obtain the offset by subtracting from the original local_t.
    std::time_t gm_offset = (gm_t - local_t);

    // Finally, get the actual time in GMT by subtracting the offset.
    std::time_t real_gm_t = local_t - gm_offset;
    return Clock::from_time_t(real_gm_t);
}

}  // namespace launchdarkly::events
