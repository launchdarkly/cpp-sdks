#pragma once

#include <launchdarkly/value.hpp>

#include <chrono>
#include <optional>
#include <string>

namespace launchdarkly::server_side::evaluation::detail {

using Clock = std::chrono::system_clock;
using Timepoint = Clock::time_point;

[[nodiscard]] std::optional<Timepoint> ParseTimestamp(
    std::string const& date,
    std::string const& format);

[[nodiscard]] std::optional<Timepoint> MillisecondsToTimepoint(double ms);

[[nodiscard]] std::optional<Timepoint> RFC3339ToTimepoint(
    std::string const& timestamp);

[[nodiscard]] std::optional<Timepoint> ToTimepoint(Value const& value);

}  // namespace launchdarkly::server_side::evaluation::detail
