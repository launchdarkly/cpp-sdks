#include "evaluation/detail/timestamp_operations.hpp"

#include <gtest/gtest.h>
#include <unordered_map>

using namespace launchdarkly::server_side::evaluation::detail;
using namespace std::chrono_literals;

struct TimestampTest {
    std::variant<std::string, double> input;
    char const* explanation;
    std::optional<Timepoint> expected;
};

class TimestampTests : public ::testing::TestWithParam<TimestampTest> {};
TEST_P(TimestampTests, ExpectedTimestampIsParsed) {
    auto const& param = GetParam();

    std::optional<Timepoint> result;
    if (param.input.index() == 1) {
        result = MillisecondsToTimepoint(std::get<double>(param.input));
    } else {
        result = RFC3339ToTimepoint(std::get<std::string>(param.input));
    }

    constexpr auto print_input =
        [](std::variant<std::string, double> const& input) {
            if (input.index() == 0) {
                return std::get<std::string>(input);
            } else {
                return std::to_string(std::get<double>(input));
            }
        };

    constexpr auto print_tp =
        [](std::optional<Timepoint> const& expected) -> std::string {
        if (expected) {
            return std::to_string(expected.value().time_since_epoch().count());
        } else {
            return "(none)";
        }
    };

    ASSERT_EQ(result, param.expected)
        << param.explanation << ": input was " << print_input(param.input)
        << ", expected " << print_tp(param.expected) << " but got "
        << print_tp(result);
}

static Timepoint BasicDate() {
    return std::chrono::system_clock::from_time_t(1577836800);
}

INSTANTIATE_TEST_SUITE_P(
    ValidTimestamps,
    TimestampTests,
    ::testing::ValuesIn({
        TimestampTest{0.0, "default constructed", Timepoint{}},
        TimestampTest{1000.0, "1 second", Timepoint{1s}},
        TimestampTest{1000.0 * 60, "60 seconds", Timepoint{60s}},
        TimestampTest{1000.0 * 60 * 60, "1 hour", Timepoint{60min}},
        TimestampTest{-1000.0, "-1 second", Timepoint{-1s}},
        TimestampTest{"2020-01-01T00:00:00Z", "with Zulu offset", BasicDate()},
        TimestampTest{"2020-01-01T00:00:00+00:00", "with normal offset",
                      BasicDate()},
        TimestampTest{"2020-01-01T01:00:00+01:00", "with 1hr offset",
                      BasicDate()},
        TimestampTest{"2020-01-01T01:00:00+01:00",
                      "with colon-delimited offset", BasicDate()},

        TimestampTest{"2020-01-01T00:00:00.123Z", "with milliseconds",
                      BasicDate() + 123ms},
        TimestampTest{"2020-01-01T00:00:00.123+00:00",
                      "with milliseconds and offset", BasicDate() + 123ms},
        TimestampTest{"2020-01-01T00:00:00.000123Z", "with microseconds ",
                      BasicDate() + 123us},
        TimestampTest{"2020-01-01T00:00:00.000123+00:00",
                      "with microseconds and offset", BasicDate() + 123us},
        TimestampTest{"2020-01-01T00:00:00.123456789Z",
                      "floor nanoseconds with zulu offset",
                      BasicDate() + 123ms + 456us},
        TimestampTest{"2020-01-01T01:00:00.123456789+01:00",
                      "floor nanoseconds with offset",
                      BasicDate() + 123ms + 456us},

    }));

INSTANTIATE_TEST_SUITE_P(
    InvalidTimestamps,
    TimestampTests,
    ::testing::ValuesIn({
        TimestampTest{0.1, "not an integer", std::nullopt},
        TimestampTest{1000.2, "not an integer", std::nullopt},
        TimestampTest{123456.789, "not an integer", std::nullopt},
        TimestampTest{-1000.5, "not an integer", std::nullopt},
        TimestampTest{"", "empty string", std::nullopt},
        TimestampTest{"2020-01-01T00:00:00/foo", "invalid offset",
                      std::nullopt},
        TimestampTest{"2020-01-01T00:00:00.0000000001Z",
                      "more than 9 digits of precision", std::nullopt},

    }));
