#include "timestamp_operations.hpp"

#include "date/date.h"

#include <sstream>

namespace launchdarkly::server_side::evaluation::detail {

std::optional<Timepoint> ToTimepoint(Value const& value) {
    if (value.Type() == Value::Type::kNumber) {
        double const& epoch_ms = value.AsDouble();
        return MillisecondsToTimepoint(epoch_ms);
    }
    if (value.Type() == Value::Type::kString) {
        std::string const& rfc3339_timestamp = value.AsString();
        return RFC3339ToTimepoint(rfc3339_timestamp);
    }
    return std::nullopt;
}

std::optional<Timepoint> MillisecondsToTimepoint(double ms) {
    if (std::trunc(ms) == ms) {
        return std::chrono::system_clock::time_point{
            std::chrono::milliseconds{static_cast<long long>(ms)}};
    }
    return std::nullopt;
}

std::optional<Timepoint> RFC3339ToTimepoint(std::string const& timestamp) {
    if (timestamp.empty()) {
        return std::nullopt;
    }
    if (timestamp.back() == 'Z') {
        return ParseTimestamp(timestamp, "%FT%H:%M:%12SZ");
    }
    return ParseTimestamp(timestamp, "%FT%H:%M:%12S%z");
}

std::optional<Timepoint> ParseTimestamp(std::string const& date,
                                        std::string const& format) {
    using namespace date;
    std::istringstream iss{date};
    Timepoint tp;
    iss >> date::parse(format, tp);
    if (iss.fail()) {
        return std::nullopt;
    }
    return tp;
}
}  // namespace launchdarkly::server_side::evaluation::detail
