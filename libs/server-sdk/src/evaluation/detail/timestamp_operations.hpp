#pragma once

#include <launchdarkly/value.hpp>

#include <chrono>
#include <optional>
#include <string>

namespace launchdarkly::server_side::evaluation::detail {

using Timepoint = std::chrono::time_point<std::chrono::system_clock,
                                          std::chrono::microseconds>;

[[nodiscard]] std::optional<Timepoint> ParseTimestamp(
    std::string const& date,
    std::string const& format);

[[nodiscard]] std::optional<Timepoint> MillisecondsToTimepoint(double ms);

[[nodiscard]] std::optional<Timepoint> RFC3339ToTimepoint(
    std::string const& timestamp);

[[nodiscard]] std::optional<Timepoint> ToTimepoint(Value const& value);

}  // namespace launchdarkly::server_side::evaluation::detail
