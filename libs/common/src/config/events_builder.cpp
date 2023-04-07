#include "config/detail/events_builder.hpp"
#include "config/detail/defaults.hpp"

#include <utility>

namespace launchdarkly::config::detail {

template <typename SDK>
EventsBuilder<SDK>::EventsBuilder() : config_(Defaults<SDK>::events()) {}

template <typename SDK>
EventsBuilder<SDK>& EventsBuilder<SDK>::capacity(std::size_t capacity) {
    config_.capacity_ = capacity;
    return *this;
}
template <typename SDK>
EventsBuilder<SDK>& EventsBuilder<SDK>::flush_interval(
    std::chrono::milliseconds interval) {
    config_.flush_interval_ = interval;
    return *this;
}

template <typename SDK>
tl::expected<Events, Error> EventsBuilder<SDK>::build() {
    if (config_.capacity() == 0) {
        return tl::unexpected(Error::kConfig_Events_ZeroCapacity);
    }
    return config_;
}

template <typename SDK>
bool operator==(EventsBuilder<SDK> const& lhs, EventsBuilder<SDK> const& rhs) {
    return lhs.config_ == rhs.config_;
}

template class EventsBuilder<detail::ClientSDK>;
template class EventsBuilder<detail::ServerSDK>;

template bool operator==(EventsBuilder<detail::ClientSDK> const&,
                         EventsBuilder<detail::ClientSDK> const&);

template bool operator==(EventsBuilder<detail::ServerSDK> const&,
                         EventsBuilder<detail::ServerSDK> const&);

}  // namespace launchdarkly::config::detail
