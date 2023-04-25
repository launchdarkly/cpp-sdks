#include "config/detail/built/events.hpp"

namespace launchdarkly::config::detail::built {

Events::Events(std::size_t capacity,
               std::chrono::milliseconds flush_interval,
               std::string path,
               bool all_attributes_private,
               AttributeReference::SetType private_attrs,
               std::chrono::milliseconds delivery_retry_delay)
    : capacity_(capacity),
      flush_interval_(flush_interval),
      path_(std::move(path)),
      all_attributes_private_(all_attributes_private),
      private_attributes_(std::move(private_attrs)),
      delivery_retry_delay_(delivery_retry_delay) {}

std::size_t Events::Capacity() const {
    return capacity_;
}

std::chrono::milliseconds Events::FlushInterval() const {
    return flush_interval_;
}

std::chrono::milliseconds Events::DeliveryRetryDelay() const {
    return delivery_retry_delay_;
}

std::string const& Events::Path() const {
    return path_;
}

bool Events::AllAttributesPrivate() const {
    return all_attributes_private_;
}

AttributeReference::SetType const& Events::PrivateAttributes() const {
    return private_attributes_;
}

bool operator==(Events const& lhs, Events const& rhs) {
    return lhs.Path() == rhs.Path() &&
           lhs.FlushInterval() == rhs.FlushInterval() &&
           lhs.Capacity() == rhs.Capacity() &&
           lhs.AllAttributesPrivate() == rhs.AllAttributesPrivate() &&
           lhs.PrivateAttributes() == rhs.PrivateAttributes() &&
           lhs.DeliveryRetryDelay() == rhs.DeliveryRetryDelay();
}
}  // namespace launchdarkly::config::detail::built
