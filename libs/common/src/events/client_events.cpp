#include "events/client_events.hpp"

namespace launchdarkly::events::client {
FeatureEventBase::FeatureEventBase(FeatureEventParams const& params)
    : creation_date(params.creation_date),
      key(params.key),
      version(params.version),
      variation(params.variation),
      value(params.value),
      default_(params.default_),
      reason(params.reason) {}

}  // namespace launchdarkly::events::client
