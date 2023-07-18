#include <launchdarkly/events/data/common_events.hpp>

namespace launchdarkly::events {
FeatureEventBase::FeatureEventBase(FeatureEventParams const& params)
    : creation_date(params.creation_date),
      key(params.key),
      version(params.version),
      variation(params.variation),
      value(params.value),
      reason(params.reason),
      default_(params.default_) {}

}  // namespace launchdarkly::events
