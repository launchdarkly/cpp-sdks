#include "timestamp_operations.hpp"

#include "timestamp.h"

#include <cmath>
#include <sstream>

namespace launchdarkly::server_side::evaluation::detail {

std::optional<Timepoint> ToTimepoint(Value const& value) {
    if (value.Type() == Value::Type::kNumber) {
        double const epoch_ms = value.AsDouble();
        return MillisecondsToTimepoint(epoch_ms);
    }
    if (value.Type() == Value::Type::kString) {
        std::string const& rfc3339_timestamp = value.AsString();
        return RFC3339ToTimepoint(rfc3339_timestamp);
    }
    return std::nullopt;
}

std::optional<Timepoint> MillisecondsToTimepoint(double ms) {
    if (ms < 0.0) {
        return std::nullopt;
    }
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

    timestamp_t ts{};
    if (timestamp_parse(timestamp.c_str(), timestamp.size(), &ts)) {
        return std::nullopt;
    }

    Timepoint epoch{};
    epoch += std::chrono::seconds{ts.sec};
    epoch +=
        std::chrono::floor<Clock::duration>(std::chrono::nanoseconds{ts.nsec});

    return epoch;
}

}  // namespace launchdarkly::server_side::evaluation::detail
