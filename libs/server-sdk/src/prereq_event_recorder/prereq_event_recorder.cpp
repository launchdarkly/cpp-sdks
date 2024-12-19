#include "prereq_event_recorder.hpp"

namespace launchdarkly::server_side {

PrereqEventRecorder::PrereqEventRecorder(std::string flag_key)
    : flag_key_(std::move(flag_key)) {}

void PrereqEventRecorder::SendAsync(events::InputEvent const event) {
    if (auto const* feat = std::get_if<events::FeatureEventParams>(&event)) {
        if (auto const prereq_of = feat->prereq_of) {
            if (*prereq_of == flag_key_) {
                prereqs_.push_back(feat->key);
            }
        }
    }
}

void PrereqEventRecorder::FlushAsync() {}

void PrereqEventRecorder::ShutdownAsync() {}

std::vector<std::string> const& PrereqEventRecorder::Prerequisites() const {
    return prereqs_;
}

std::vector<std::string>&& PrereqEventRecorder::TakePrerequisites() && {
    return std::move(prereqs_);
}

}  // namespace launchdarkly::server_side
