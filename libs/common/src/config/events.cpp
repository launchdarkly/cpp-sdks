#include "config/detail/events.hpp"

namespace launchdarkly::config::detail {

Events::Events(std::size_t capacity,
               std::chrono::milliseconds flush_interval,
               std::string path,
               AttributePolicy policy)
    : capacity_(capacity),
      flush_interval_(flush_interval),
      path_(std::move(path)),
      attribute_policy_(policy) {}

std::size_t Events::capacity() const {
    return capacity_;
}
std::chrono::milliseconds Events::flush_interval() const {
    return flush_interval_;
}
std::string const& Events::path() const {
    return path_;
}
AttributePolicy Events::attribute_policy() const {
    return attribute_policy_;
}

bool operator==(Events const& lhs, Events const& rhs) {
    return lhs.path() == rhs.path() &&
           lhs.flush_interval() == rhs.flush_interval() &&
           lhs.capacity() == rhs.capacity() &&
           lhs.attribute_policy() == rhs.attribute_policy();
}
}  // namespace launchdarkly::config::detail
