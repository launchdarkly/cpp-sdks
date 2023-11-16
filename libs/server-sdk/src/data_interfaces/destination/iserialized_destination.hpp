#pragma once

#include <launchdarkly/server_side/integrations/iserialized_item_kind.hpp>
#include <launchdarkly/server_side/integrations/serialized_item_descriptor.hpp>

#include <string>
#include <utility>
#include <vector>

namespace launchdarkly::server_side::data_interfaces {

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
     * \brief Initializes the destination with data.
     * \param sdk_data_set A series of collections, where each collection is
     * named by an ISerializedItemKind and contains a list of key/value pairs
     * representing the key of the item and the serialized form of the item.
     * \return InitResult::kSuccess if all
     * data items were stored, or InitResult::kError if any error occoured.
     */
    [[nodiscard]] virtual InitResult Init(
        std::vector<ItemCollection> sdk_data_set) = 0;

    /**
     * \brief Upserts a single item (update if exist, insert if not.)
     * \param kind The item kind.
     * \param key The item key.
     * \param item Serialized form of the item. The item should be deleted if
     * the SerializedItem's 'deleted' bool is true. \return
     * UpsertResult::kSuccess if the operation was successful.
     * UpsertResult::kError if an error occured. Otherwise,
     * UpsertResult::kNotUpdated if the existing item version was greater than
     * the version passed in.
     */
    [[nodiscard]] virtual UpsertResult Upsert(
        std::string const& kind,
        std::string const& key,
        integrations::SerializedItemDescriptor item) = 0;

    /**
     * \return Identity of the destination. Used in logs.
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
