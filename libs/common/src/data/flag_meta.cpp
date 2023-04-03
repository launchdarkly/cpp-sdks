#include "data/flag_meta.hpp"

namespace launchdarkly {

long FlagMeta::version() const {
    return version_;
}

bool FlagMeta::track_events() const {
    return track_events_;
}

bool FlagMeta::track_reason() const {
    return track_reason_;
}

std::optional<long> FlagMeta::track_events_until_date() const {
    return track_events_until_date_;
}

EvaluationDetail const& FlagMeta::detail() const {
    return detail_;
}

}  // namespace launchdarkly
