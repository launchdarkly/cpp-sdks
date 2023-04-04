#pragma once

#include <boost/json/value.hpp>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include "attribute_reference.hpp"
#include "context.hpp"
#include "value.hpp"

#include <cstdint>

namespace launchdarkly::events {

// TODO: Replace with actual types when available.
using Value = launchdarkly::Value;
using VariationIndex = size_t;
using Reason = std::string;
using Context = launchdarkly::Context;
using Json = boost::json::value;
using Version = std::uint64_t;

struct BaseEvent {
    std::chrono::milliseconds creation_date;
    bool inline_;
    bool all_attributes_private;
    std::unordered_set<AttributeReference> global_private_attributes;
    Context context;
};

struct FeatureEvent {
    BaseEvent base;
    std::string key;
    Value value;
    std::optional<VariationIndex> variation;
    Value default_;
    std::optional<Reason> reason;
    std::optional<Version> version;
};

struct PrerequisiteEvent {
    BaseEvent base;
    std::string key;
    Value value;
    std::optional<VariationIndex> variation;
    std::optional<Reason> reason;
    std::optional<Version> version;
    std::string prereq_of;
};

using IndexEvent = BaseEvent;

struct IdentifyEvent {
    BaseEvent base;
    std::string key;
};

struct CustomEvent {
    BaseEvent base;
    std::string key;
    std::optional<Json> data;
    std::optional<double> metric_value;
};

struct DebugEvent {
    FeatureEvent feature;
};

struct VariationSummary {
    std::size_t count;
    Value value;
};

struct VariationKey {
    std::optional<Version> version;
    std::optional<VariationIndex> variation;
};

struct FlagSummary {
    std::unordered_map<VariationKey, VariationSummary> counters;
    Value default_;
    std::unordered_set<std::string> context_kinds;
};
struct SummaryEvent {
    std::chrono::milliseconds start_date;
    std::chrono::milliseconds end_date;
    std::unordered_map<std::string, FlagSummary> features;
};

using InputEvent = std::variant<FeatureEvent, IdentifyEvent, CustomEvent>;

using OutputEvent = std::variant<IndexEvent,
                                 DebugEvent,
                                 FeatureEvent,
                                 IdentifyEvent,
                                 CustomEvent,
                                 SummaryEvent>;

}  // namespace launchdarkly::events
