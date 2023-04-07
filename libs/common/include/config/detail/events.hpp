#pragma once

#include <chrono>
#include <cstddef>
#include <string>
namespace launchdarkly::config::detail {

enum class AttributePolicy {
    Default = 0,
    AllPrivate = 1,
};
struct Events {
   public:
    template <typename SDK>
    friend class EventsBuilder;
    Events(std::size_t capacity,
           std::chrono::milliseconds flush_interval,
           std::string path,
           AttributePolicy policy);
    std::size_t capacity() const;
    std::chrono::milliseconds flush_interval() const;
    std::string const& path() const;
    AttributePolicy attribute_policy() const;

   private:
    std::size_t capacity_;
    std::chrono::milliseconds flush_interval_;
    std::string path_;
    AttributePolicy attribute_policy_;
};

bool operator==(Events const& lhs, Events const& rhs);

}  // namespace launchdarkly::config::detail
