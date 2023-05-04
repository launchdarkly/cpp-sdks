#pragma once

#include "events/common_events.hpp"

namespace launchdarkly::events::client {

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

}  // namespace launchdarkly::events::client
