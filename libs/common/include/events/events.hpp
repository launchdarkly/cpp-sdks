#pragma once

#include <boost/json/value.hpp>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include "attribute_reference.hpp"
#include "context.hpp"
#include "data/evaluation_reason.hpp"
#include "value.hpp"

#include <cstdint>

namespace launchdarkly::events {

// TODO: Replace with actual types when available.
using Value = launchdarkly::Value;
using VariationIndex = size_t;
using Reason = EvaluationReason;
using Context = launchdarkly::Context;
using Json = boost::json::value;
using Version = std::uint64_t;
using ContextKeys = std::map<std::string, std::string>;

struct Date {
    std::chrono::system_clock::time_point t;
};

struct FeatureEventFields {
    std::string key;
    Date creation_date;
    Value value;
    std::optional<VariationIndex> variation;
    Value default_;
    std::optional<Reason> reason;
    Version version;
    std::optional<std::string> prereq_of;
};

struct FeatureEvent {
    FeatureEventFields base;
    ContextKeys context_keys;
};

struct DebugEvent {
    FeatureEventFields base;
    Context context;
};

struct IdentifyEvent {
    Date creation_date;
    Context context;
};

struct IndexEvent : public IdentifyEvent {};

struct CustomEvent {
    Date creation_date;
    std::string key;
    ContextKeys context_keys;
    std::optional<Value> data;
    std::optional<double> metric_value;
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

using InputEvent = std::variant<FeatureEvent, IdentifyEvent, CustomEvent>;

using OutputEvent = std::
    variant<IndexEvent, DebugEvent, FeatureEvent, IdentifyEvent, CustomEvent>;

}  // namespace launchdarkly::events
