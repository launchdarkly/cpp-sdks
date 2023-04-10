#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <unordered_map>
#include "attribute_reference.hpp"
namespace launchdarkly::config::detail {

enum class TransportSecurity {
    None = 0,
    TLS = 1,
};

struct Events {
   public:
    template <typename SDK>
    friend class EventsBuilder;
    Events(std::size_t capacity,
           std::chrono::milliseconds flush_interval,
           std::string path,
           bool all_attributes_private,
           AttributeReference::SetType private_attrs,
           TransportSecurity security);
    std::size_t capacity() const;
    std::chrono::milliseconds flush_interval() const;
    std::string const& path() const;
    bool all_attributes_private() const;
    AttributeReference::SetType const& private_attributes() const;
    TransportSecurity transport_security() const;

   private:
    std::size_t capacity_;
    std::chrono::milliseconds flush_interval_;
    std::string path_;
    bool all_attributes_private_;
    AttributeReference::SetType private_attributes_;

    TransportSecurity transport_security_;
};

bool operator==(Events const& lhs, Events const& rhs);

}  // namespace launchdarkly::config::detail
