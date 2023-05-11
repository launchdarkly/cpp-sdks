#include <launchdarkly/events/client_events.hpp>

namespace launchdarkly::events::client {
FeatureEventBase::FeatureEventBase(FeatureEventParams const& params)
    : creation_date(params.creation_date),
      key(params.key),
      version(params.version),
      variation(params.variation),
      value(params.value),
      reason(params.reason),
      default_(params.default_) {}

}  // namespace launchdarkly::events::client
