#pragma once

#include <launchdarkly/serialization/json_errors.hpp>

#include <boost/json.hpp>
#include <tl/expected.hpp>

#include <memory>
#include <optional>
#include <ostream>
#include <unordered_map>

namespace launchdarkly::data_model {
/**
 * An item descriptor is an abstraction that allows for Flag data to be
 * handled using the same type in both a put or a patch.
 */
template <typename T>
struct ItemDescriptor {
    /**
     * The version number of this data, provided by the SDK.
     */
    uint64_t version;

    /**
     * The data item, or nullopt if this is a deleted item placeholder.
     */
    std::optional<T> item;

    explicit ItemDescriptor(uint64_t version);

    explicit ItemDescriptor(T item);

    ItemDescriptor(ItemDescriptor const&) = default;
    ItemDescriptor(ItemDescriptor&&) = default;
    ItemDescriptor& operator=(ItemDescriptor const&) = default;
    ItemDescriptor& operator=(ItemDescriptor&&) = default;
    ~ItemDescriptor() = default;
};

template <typename T>
bool operator==(ItemDescriptor<T> const& lhs, ItemDescriptor<T> const& rhs) {
    return lhs.version == rhs.version && lhs.item == rhs.item;
}

template <typename T>
std::ostream& operator<<(std::ostream& out,
                         ItemDescriptor<T> const& descriptor) {
    out << "{";
    out << " version: " << descriptor.version;
    if (descriptor.item.has_value()) {
        out << " item: " << descriptor.item.value();
    } else {
        out << " item: <nullopt>";
    }
    return out;
}

template <typename T>
ItemDescriptor<T>::ItemDescriptor(uint64_t version) : version(version) {}

template <typename T>
ItemDescriptor<T>::ItemDescriptor(T item)
    : version(item.Version()), item(std::move(item)) {}

}  // namespace launchdarkly::data_model
