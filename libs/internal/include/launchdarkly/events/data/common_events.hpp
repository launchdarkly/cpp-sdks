#pragma once

#include <launchdarkly/context.hpp>
#include <launchdarkly/data/evaluation_reason.hpp>
#include <launchdarkly/data/evaluation_result.hpp>

#include <boost/json/value.hpp>

#include <chrono>
#include <cstdint>
#include <variant>

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

struct TrackEventParams {
    Date creation_date;
    std::string key;
    ContextKeys context_keys;
    std::optional<Value> data;
    std::optional<double> metric_value;
};

// Track (custom) events are directly serialized from their parameters.
using TrackEvent = TrackEventParams;

struct IdentifyEventParams {
    Date creation_date;
    Context context;
};

struct IdentifyEvent {
    Date creation_date;
    EventContext context;
};

struct FeatureEventParams {
    Date creation_date;
    std::string key;
    Context context;
    Value value;
    Value default_;
    std::optional<Version> version;
    std::optional<VariationIndex> variation;
    std::optional<Reason> reason;
    bool require_full_event;
    std::optional<Date> debug_events_until_date;
};

struct FeatureEventBase {
    Date creation_date;
    std::string key;
    std::optional<Version> version;
    std::optional<VariationIndex> variation;
    Value value;
    std::optional<Reason> reason;
    Value default_;

    explicit FeatureEventBase(FeatureEventParams const& params);
};

struct FeatureEvent : public FeatureEventBase {
    ContextKeys context_keys;
};

struct DebugEvent : public FeatureEventBase {
    EventContext context;
};

}  // namespace launchdarkly::events