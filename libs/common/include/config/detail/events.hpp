#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <unordered_map>
#include "attribute_reference.hpp"
namespace launchdarkly::config::detail {

struct Events {
   public:
    template <typename SDK>
    friend class EventsBuilder;
    /**
     * Constructs configuration for the event subsystem.
     * @param capacity How many events can queue in memory before new events
     * are dropped.
     * @param flush_interval How often events are automatically flushed to
     * LaunchDarkly.
     * @param path The path component of the LaunchDarkly event delivery
     * endpoint.
     * @param all_attributes_private Whether all attributes should be treated as
     * private or not.
     * @param private_attrs Which attributes should be treated as private, if
     * all_attributes_private is false.
     * @param security Whether a plaintext or encrypted client should be used
     * for event delivery.
     */
    Events(std::size_t capacity,
           std::chrono::milliseconds flush_interval,
           std::string path,
           bool all_attributes_private,
           AttributeReference::SetType private_attrs);

    /**
     * Capacity of the event processor.
     */
    [[nodiscard]] std::size_t capacity() const;

    /**
     * Flush interval of the event processor, in milliseconds.
     */
    [[nodiscard]] std::chrono::milliseconds flush_interval() const;

    /**
     * Path component of the LaunchDarkly event delivery endpoint.
     */
    [[nodiscard]] std::string const& path() const;

    /**
     * Whether all attributes should be considered private or not.
     */
    [[nodiscard]] bool all_attributes_private() const;

    /**
     * Set of individual attributes that should be considered private.
     */
    [[nodiscard]] AttributeReference::SetType const& private_attributes() const;

   private:
    std::size_t capacity_;
    std::chrono::milliseconds flush_interval_;
    std::string path_;
    bool all_attributes_private_;
    AttributeReference::SetType private_attributes_;
};

bool operator==(Events const& lhs, Events const& rhs);

}  // namespace launchdarkly::config::detail
