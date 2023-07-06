#pragma once

#include <launchdarkly/attribute_reference.hpp>

#include <chrono>
#include <cstddef>
#include <string>
#include <unordered_map>

namespace launchdarkly::config::shared::builders {
template <typename T>
class EventsBuilder;
}

namespace launchdarkly::config::shared::built {

class Events final {
   public:
    template <typename SDK>
    friend class builders::EventsBuilder;
    /**
     * Constructs configuration for the event subsystem.
     * @param enabled If event-sending is enabled. If false, no events will be
     * sent to LaunchDarkly.
     * @param capacity How many events can queue in memory before new events
     * are dropped.
     * @param flush_interval How often events are automatically flushed to
     * LaunchDarkly.
     * @param path The path component of the LaunchDarkly event delivery
     * endpoint.
     * @param all_attributes_private Whether all attributes should be treated as
     * private or not.
     * @param private_attrs Which attributes should be treated as private, if
     * AllAttributesPrivate is false.
     * @param delivery_retry_delay How long to wait before a redelivery attempt
     * should be made.
     * @param flush_workers How many workers to use for concurrent event
     * delivery.
     * @param context_keys_cache_capacity Max number of unique context keys to
     * hold in LRU cache used for context deduplication when generating index
     * events.
     */
    Events(bool enabled,
           std::size_t capacity,
           std::chrono::milliseconds flush_interval,
           std::string path,
           bool all_attributes_private,
           AttributeReference::SetType private_attrs,
           std::chrono::milliseconds delivery_retry_delay,
           std::size_t flush_workers,
           std::optional<std::size_t> context_keys_cache_capacity);

    /**
     * Returns true if event-sending is enabled.
     */
    [[nodiscard]] bool Enabled() const;

    /**
     * Capacity of the event processor.
     */
    [[nodiscard]] std::size_t Capacity() const;

    /**
     * Flush interval of the event processor, in milliseconds.
     */
    [[nodiscard]] std::chrono::milliseconds FlushInterval() const;

    /*
     * If an event payload fails to be delivered and can be retried, how long
     * to wait before retrying.
     */
    [[nodiscard]] std::chrono::milliseconds DeliveryRetryDelay() const;

    /**
     * Path component of the LaunchDarkly event delivery endpoint.
     */
    [[nodiscard]] std::string const& Path() const;

    /**
     * Whether all attributes should be considered private or not.
     */
    [[nodiscard]] bool AllAttributesPrivate() const;

    /**
     * Set of individual attributes that should be considered private.
     */
    [[nodiscard]] AttributeReference::SetType const& PrivateAttributes() const;

    /**
     * Number of flush workers used for concurrent event delivery.
     */
    [[nodiscard]] std::size_t FlushWorkers() const;

    /**
     * Max number of unique context keys to hold in LRU cache used for context
     * deduplication when generating index events.
     * @return Max, or std::nullopt if not applicable.
     */
    [[nodiscard]] std::optional<std::size_t> ContextKeysCacheCapacity() const;

   private:
    bool enabled_;
    std::size_t capacity_;
    std::chrono::milliseconds flush_interval_;
    std::string path_;
    bool all_attributes_private_;
    AttributeReference::SetType private_attributes_;
    std::chrono::milliseconds delivery_retry_delay_;
    std::size_t flush_workers_;
    std::optional<std::size_t> context_keys_cache_capacity_;
};

bool operator==(Events const& lhs, Events const& rhs);

}  // namespace launchdarkly::config::shared::built
