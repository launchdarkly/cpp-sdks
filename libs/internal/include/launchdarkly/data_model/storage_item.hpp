#pragma once

#include <boost/serialization/strong_typedef.hpp>

#include <variant>

namespace launchdarkly::data_model {

/** Represents the version number of a deleted item. */
BOOST_STRONG_TYPEDEF(std::uint64_t, Tombstone);

/** Represents an item that was retrieved from a data store, or
 * a tombstone representing the deletion of that item. */
template <typename Item>
using StorageItem = std::variant<Item, Tombstone>;

}  // namespace launchdarkly::data_model
