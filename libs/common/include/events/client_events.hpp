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
    EvaluationResult eval_result;
};

struct FeatureEventBase {
    Date creation_date;
    std::string key;
    Version version;
    std::optional<VariationIndex> variation;
    Value value;
    std::optional<Reason> reason;
    Value default_;
};

struct FeatureEvent : public FeatureEventBase {
    ContextKeys context_keys;
};

struct DebugEvent : public FeatureEventBase {
    EventContext context;
};

}  // namespace launchdarkly::events::client
