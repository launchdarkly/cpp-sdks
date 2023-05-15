#include <launchdarkly/config/shared/builders/events_builder.hpp>
#include "launchdarkly/config/shared/defaults.hpp"

#include <utility>

namespace launchdarkly::config::shared::builders {

template <typename SDK>
EventsBuilder<SDK>::EventsBuilder() : config_(Defaults<SDK>::Events()) {}

template <typename SDK>
EventsBuilder<SDK>& EventsBuilder<SDK>::Enabled(bool enabled) {
    config_.enabled_ = enabled;
    return *this;
}

template <typename SDK>
EventsBuilder<SDK>& EventsBuilder<SDK>::Disable() {
    return Enabled(false);
}

template <typename SDK>
EventsBuilder<SDK>& EventsBuilder<SDK>::Capacity(std::size_t capacity) {
    config_.capacity_ = capacity;
    return *this;
}

template <typename SDK>
EventsBuilder<SDK>& EventsBuilder<SDK>::FlushInterval(
    std::chrono::milliseconds interval) {
    config_.flush_interval_ = interval;
    return *this;
}

template <typename SDK>
EventsBuilder<SDK>& EventsBuilder<SDK>::AllAttributesPrivate(bool value) {
    config_.all_attributes_private_ = value;
    return *this;
}

template <typename SDK>
EventsBuilder<SDK>& EventsBuilder<SDK>::PrivateAttributes(
    AttributeReference::SetType attributes) {
    config_.private_attributes_ = std::move(attributes);
    return *this;
}

template <typename SDK>
tl::expected<built::Events, Error> EventsBuilder<SDK>::Build() {
    if (config_.Capacity() == 0) {
        return tl::unexpected(Error::kConfig_Events_ZeroCapacity);
    }
    return config_;
}

template <typename SDK>
bool operator==(EventsBuilder<SDK> const& lhs, EventsBuilder<SDK> const& rhs) {
    return lhs.config_ == rhs.config_;
}

template class EventsBuilder<config::ClientSDK>;
template class EventsBuilder<config::ServerSDK>;

template bool operator==(EventsBuilder<config::ClientSDK> const&,
                         EventsBuilder<config::ClientSDK> const&);

template bool operator==(EventsBuilder<config::ServerSDK> const&,
                         EventsBuilder<config::ServerSDK> const&);

}  // namespace launchdarkly::config::shared::builders
