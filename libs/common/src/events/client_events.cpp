#include "events/client_events.hpp"

namespace launchdarkly::events::client {
FeatureEventBase::FeatureEventBase(FeatureEventParams const& params)
    : creation_date(params.creation_date),
      key(params.key),
      version(params.eval_result.version()),
      variation(params.eval_result.detail().variation_index()),
      value(params.eval_result.detail().value()),
      default_(params.default_) {
    // TODO(cwaldren): should also add the reason if the
    // variation method was VariationDetail().
    if (params.eval_result.track_reason()) {
        reason = params.eval_result.detail().reason();
    }
}

}  // namespace launchdarkly::events::client
