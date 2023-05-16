#pragma once

#include <launchdarkly/attribute_reference.hpp>
#include <launchdarkly/config/shared/built/events.hpp>
#include <launchdarkly/error.hpp>

#include "tl/expected.hpp"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace launchdarkly::config::shared::builders {

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
     * Specify if event-sending should be enabled or not. By default,
     * events are enabled.
     * @param enabled True to enable.
     * @return Reference to this builder.
     */
    EventsBuilder& Enabled(bool enabled);

    /**
     * Alias for Enabled(false).
     * @return Reference to this builder.
     */
    EventsBuilder& Disable();

    /**
     * Sets the capacity of the event processor. When more events are generated
     * within the processor's flush interval than this value, events will be
     * dropped.
     * @param capacity Event queue capacity.
     * @return Reference to this builder.
     */
    EventsBuilder& Capacity(std::size_t capacity);

    /**
     * Sets the flush interval of the event processor. The processor queues
     * outgoing events based on the capacity parameter; these events are then
     * delivered based on the flush interval.
     * @param interval Interval between automatic flushes.
     * @return Reference to this builder.
     */
    EventsBuilder& FlushInterval(std::chrono::milliseconds interval);

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
     * - call @ref PrivateAttributes.
     *
     * (3) To specify private attributes on a per-context basis, it is not
     * necessary to call either of these methods, as the default behavior is to
     * treat all attributes as non-private unless otherwise specified.
     *
     * @param value True for behavior of (1), false for default behavior of (2)
     * or (3).
     * @return Reference to this builder.
     */
    EventsBuilder& AllAttributesPrivate(bool all_attributes_private);

    /**
     * Specify that a set of attributes are private.
     * @return Reference to this builder.
     */
    EventsBuilder& PrivateAttributes(AttributeReference::SetType private_attrs);

    /**
     * Builds Events configuration, if the configuration is valid.
     * @return Events config, or error.
     */
    [[nodiscard]] tl::expected<built::Events, Error> Build();

   private:
    built::Events config_;
};

}  // namespace launchdarkly::config::shared::builders
