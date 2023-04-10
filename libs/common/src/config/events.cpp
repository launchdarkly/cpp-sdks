#include "config/detail/events.hpp"

namespace launchdarkly::config::detail {

Events::Events(std::size_t capacity,
               std::chrono::milliseconds flush_interval,
               std::string path,
               bool all_attributes_private,
               AttributeReference::SetType private_attrs,
               TransportSecurity transport_security)
    : capacity_(capacity),
      flush_interval_(flush_interval),
      path_(std::move(path)),
      all_attributes_private_(all_attributes_private),
      private_attributes_(std::move(private_attrs)),
      transport_security_(transport_security) {}

std::size_t Events::capacity() const {
    return capacity_;
}
std::chrono::milliseconds Events::flush_interval() const {
    return flush_interval_;
}
std::string const& Events::path() const {
    return path_;
}

bool Events::all_attributes_private() const {
    return all_attributes_private_;
}

AttributeReference::SetType const& Events::private_attributes() const {
    return private_attributes_;
}

TransportSecurity Events::transport_security() const {
    return transport_security_;
}

bool operator==(Events const& lhs, Events const& rhs) {
    return lhs.path() == rhs.path() &&
           lhs.flush_interval() == rhs.flush_interval() &&
           lhs.capacity() == rhs.capacity() &&
           lhs.all_attributes_private() == rhs.all_attributes_private() &&
           lhs.private_attributes() == rhs.private_attributes();
}
}  // namespace launchdarkly::config::detail
