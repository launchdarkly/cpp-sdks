#pragma once

#include <tl/expected.hpp>
#include "attribute_reference.hpp"
#include "config/detail/events.hpp"
#include "error.hpp"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

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
     * @param capacity Event queue capacity.
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
     * Attribute privacy indicates whether or not attributes should be
     * retained by LaunchDarkly after being sent upon initialization,
     * and if attributes should later be sent in events.
     *
     * Attribute privacy may be specified in 3 ways:
     *
     * (1) To specify that all attributes should be considered private - not
     * just those designated private on a per-context basis - call this method
     * with true as the parameter.
     *
     * (2) To specify that a specific set of attributes should be considered
     * private - in addition to those designated private on a per-context basis
     * - call @ref private_attributes.
     *
     * (3) To specify private attributes on a per-context basis, it is not
     * necessary to call either of these methods, as the default behavior is to
     * treat all attributes as non-private unless otherwise specified.
     *
     * @param value True for behavior of (1), false for default behavior of (2)
     * or (3).
     * @return Reference to this builder.
     */
    EventsBuilder& all_attributes_private(bool);

    /**
     * Specify that a set of attributes are private.
     * @return Reference to this builder.
     */
    EventsBuilder& private_attributes(
        AttributeReference::SetType private_attrs);

    /**
     * Builds Events configuration, if the configuration is valid.
     * @return Events config, or error.
     */
    [[nodiscard]] tl::expected<Events, Error> build();

   private:
    Events config_;
};

}  // namespace launchdarkly::config::detail
