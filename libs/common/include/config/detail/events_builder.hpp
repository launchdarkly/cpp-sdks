#pragma once

#include "config/detail/events.hpp"
#include "error.hpp"

#include <tl/expected.hpp>

#include <memory>
#include <optional>
#include <string>

namespace launchdarkly::config::detail {

template <typename SDK>
class EventsBuilder;

template <typename SDK>
bool operator==(EventsBuilder<SDK> const& lhs, EventsBuilder<SDK> const& rhs);

/**
 * EventsBuilder allows for specification of parameters related to the
 * SDK's event processor.
 *
 * @tparam SDK Type of SDK, such as ClientSDK or ServerSDK.
 */
template <typename SDK>
class EventsBuilder {
   public:
    friend bool operator==
        <SDK>(EventsBuilder<SDK> const& lhs, EventsBuilder<SDK> const& rhs);
    /**
     * Constructs an EventsBuilder.
     */
    EventsBuilder();

    /**
     * Sets the capacity of the event processor. When more events are generated
     * within the processor's flush interval than this value, events will be
     * dropped.
     * @param capacity Event capacity.
     * @return Reference to this builder.
     */
    EventsBuilder& capacity(std::size_t capacity);

    /**
     * Sets the flush interval of the event processor. The processor queues
     * outgoing events based on the capacity parameter; these events are then
     * delivered based on the flush interval.
     * @param interval Interval between automatic flushes.
     * @return Reference to this builder.
     */
    EventsBuilder& flush_interval(std::chrono::milliseconds interval);

    /**
     * Builds Events configuration, if the configuration is valid. If not,
     * returns an error.
     * @return Events config, or error.
     */
    [[nodiscard]] tl::expected<Events, Error> build();

   private:
    Events config_;
};

}  // namespace launchdarkly::config::detail
