#pragma once

#include <chrono>
#include <cstdint>
#include <variant>

#include <boost/json/value.hpp>
#include "context.hpp"
#include "data/evaluation_reason.hpp"
#include "data/evaluation_result.hpp"

namespace launchdarkly::events {

using Value = launchdarkly::Value;
using VariationIndex = size_t;
using Reason = EvaluationReason;
using Result = EvaluationResult;
using Context = launchdarkly::Context;
using EventContext = boost::json::value;
using Version = std::uint64_t;
using ContextKeys = std::map<std::string, std::string>;

struct Date {
    std::chrono::system_clock::time_point t;
};

struct VariationSummary {
    std::size_t count;
    Value value;
};

struct VariationKey {
    Version version;
    std::optional<VariationIndex> variation;

    struct Hash {
        auto operator()(VariationKey const& p) const -> size_t {
            if (p.variation) {
                return std::hash<Version>{}(p.version) ^
                       std::hash<VariationIndex>{}(*p.variation);
            } else {
                return std::hash<Version>{}(p.version);
            }
        }
    };
};

struct TrackEventParams {
    Date creation_date;
    std::string key;
    ContextKeys context_keys;
    std::optional<Value> data;
    std::optional<double> metric_value;
};

// Track (custom) events are directly serialized from their parameters.
using TrackEvent = TrackEventParams;

}  // namespace launchdarkly::events
