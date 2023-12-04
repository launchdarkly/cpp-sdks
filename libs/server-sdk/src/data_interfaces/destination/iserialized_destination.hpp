#pragma once

#include <launchdarkly/server_side/integrations/data_reader/iserialized_item_kind.hpp>
#include <launchdarkly/server_side/integrations/data_reader/serialized_item_descriptor.hpp>

#include <string>
#include <utility>
#include <vector>

namespace launchdarkly::server_side::data_interfaces {

/**
 * @brief This interface is used for persisting data to databases, or any other
 * component that can store feature flag / segment data.
 *
 * The SDK automatically converts between its in-memory data model and a
 * serialized string form, which is what this interface interacts with.
 *
 * Each item in the store is conceptually a SerializedItemDescriptor containing
 * a version and the serialized form. The serialized form might represent a
 * flag/segment, or a "tombstone" representing the (absence) of an item.
 *
 * It's possible to satisfy the interface in two ways:
 *
 * 1. The Destination can store the version number, deleted state, and item
 * separately. This is preferred because it avoids the need to deserialize the
 * entire item just to inspect the version/deleted state when performing an
 * Upsert operation. If implementing this strategy, the Destination may ignore
 * deleted SerializeItemDescriptor's serializedItem members on Upserts.
 *
 * 2. If there's no way to store the version number, deleted state, and item
 * separately in an efficient way, then the store may instead persist the
 * serializedItem as-is during an Upsert. The item will contain a "tombstone"
 * representation which the SDK will later use to determine if the item is
 * deleted or not.
 */
class ISerializedDestination {
   public:
    enum class InitResult {
        /**
         * The init operation completed successfully.
         */
        kSuccess,

        /**
         * There was an error with the init operation.
         */
        kError,
    };

    enum class UpsertResult {
        /**
         * The upsert completed successfully.
         */
        kSuccess,

        /**
         * There was an error with the upsert operation.
         */
        kError,

        /**
         * The upsert did not encounter errors, but the version of the
         * existing item was greater than that the version of the upsert item.
         */
        kNotUpdated
    };

    using Key = std::string;

    template <typename T>
    using Keyed = std::pair<Key, T>;

    using OrderedNamepace =
        std::vector<Keyed<integrations::SerializedItemDescriptor>>;

    using ItemCollection =
        std::pair<integrations::ISerializedItemKind const&, OrderedNamepace>;

    /**
     * @brief Overwrites the Destination's contents with a set of items for each
     * collection. All previous data should be disgraded regardless of
     * versioning.
     *
     * The update should be done atomically. If that's not possible, the store
     * must first add or update each item in the same order that they are given
     * in the input data, and then delete any previously stored items that were
     * not in the input data.
     *
     * @param sdk_data_set A series of collections, where each collection is
     * named by an ISerializedItemKind and contains a list of key/value pairs
     * representing the key of the item and the serialized form of the item.
     * @return InitResult::kSuccess if all
     * data items were stored, or InitResult::kError if any error occoured.
     */
    [[nodiscard]] virtual InitResult Init(
        std::vector<ItemCollection> sdk_data_set) = 0;

    /**
     * @brief Upserts a single item (update if exist, insert if not.)
     *
     * If the given key already exists in the collection named by kind,
     * then the Destination must check the version number corresponding to that
     * key. Note that the item corresponding to that key may be a tombstone
     * representing an absent item.
     *
     * If the version of the existing item is >= the version of the new item,
     * return UpsertResult::kNotUpdated. If the Destination can't determine the
     * version number of the existing item without full deserialization, then it
     * may call integrations::ISerializedItemKind::Version on the data to obtain
     * it.
     *
     * If the given item's deleted flag is true, the Destination must persist
     * this fact. It can either store a tombstone (value of serializedItem), or
     * if deletion state is stored separate from the item, it can use that
     * mechanism. In any case, it should not delete/forget about the item.
     *
     * @param kind The item kind.
     * @param key The item key.
     * @param item Serialized form of the item.
     * @return
     * UpsertResult::kSuccess if the operation was successful.
     * UpsertResult::kError if an error occured. Otherwise,
     * UpsertResult::kNotUpdated if the existing item version was greater than
     * the version passed in.
     */
    [[nodiscard]] virtual UpsertResult Upsert(
        integrations::ISerializedItemKind const& kind,
        std::string const& key,
        integrations::SerializedItemDescriptor item) = 0;

    /**
     * @return Identity of the destination. Used in logs.
     */
    [[nodiscard]] virtual std::string const& Identity() const = 0;

    ISerializedDestination(ISerializedDestination const& item) = delete;
    ISerializedDestination(ISerializedDestination&& item) = delete;
    ISerializedDestination& operator=(ISerializedDestination const&) = delete;
    ISerializedDestination& operator=(ISerializedDestination&&) = delete;
    virtual ~ISerializedDestination() = default;

   protected:
    ISerializedDestination() = default;
};
}  // namespace launchdarkly::server_side::data_interfaces
