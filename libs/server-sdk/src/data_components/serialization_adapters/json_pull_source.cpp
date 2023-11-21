#include "json_pull_source.hpp"

#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_segment.hpp>

#include <launchdarkly/server_side/integrations/iserialized_item_kind.hpp>

#include <boost/json.hpp>

namespace launchdarkly::server_side::data_components {

JsonSource::JsonSource(data_interfaces::ISerializedDataPullSource& json_source)
    : flag_kind_(), segment_kind_(), source_(json_source) {}

data_interfaces::IPullSource::Single<data_model::FlagDescriptor>
JsonSource::GetFlag(std::string const& key) const {
    return Deserialize<data_model::Flag>(flag_kind_, key);
}

data_interfaces::IPullSource::Single<data_model::SegmentDescriptor>
JsonSource::GetSegment(std::string const& key) const {
    return Deserialize<data_model::Segment>(segment_kind_, key);
}

data_interfaces::IPullSource::Collection<data_model::FlagDescriptor>
JsonSource::AllFlags() const {
    // TODO: deserialize then return
    data_interfaces::ISerializedDataPullSource::AllResult result =
        source_.All(flag_kind_);
    if (!result) {
        return tl::make_unexpected(result.error().message);
    }

    Collection<data_model::FlagDescriptor> flags;
    for (auto [key, val] : *result) {
        // TODO
    }
    return flags;
}

data_interfaces::IPullSource::Collection<data_model::SegmentDescriptor>
JsonSource::AllSegments() const {
    // TODO: deserialize then return

    data_interfaces::ISerializedDataPullSource::AllResult result =
        source_.All(segment_kind_);
}

std::string const& JsonSource::Identity() const {
    return source_.Identity();
}

}  // namespace launchdarkly::server_side::data_components
