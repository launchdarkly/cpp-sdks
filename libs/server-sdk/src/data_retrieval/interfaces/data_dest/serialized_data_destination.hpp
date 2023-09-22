#pragma once

#include "serialized_descriptors.hpp"

#include <launchdarkly/data_model/descriptors.hpp>
#include <launchdarkly/data_model/sdk_data_set.hpp>

namespace launchdarkly::server_side::data_system {

class ISerializedDataDestination {
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

    using ItemKey = std::string;
    using KeyItemPair = std::pair<ItemKey, SerializedItemDescriptor>;
    using OrderedNamepace = std::vector<KeyItemPair>;
    using KindCollectionPair =
        std::pair<IPersistentKind const&, OrderedNamepace>;
    using OrderedData = std::vector<KindCollectionPair>;

    virtual InitResult Init(OrderedData sdk_data_set) = 0;

    virtual UpsertResult Upsert(std::string const& kind,
                                std::string const& key,
                                SerializedItemDescriptor item) = 0;

    virtual std::string Identity() const = 0;

    ISerializedDataDestination(ISerializedDataDestination const& item) = delete;
    ISerializedDataDestination(ISerializedDataDestination&& item) = delete;
    ISerializedDataDestination& operator=(ISerializedDataDestination const&) =
        delete;
    ISerializedDataDestination& operator=(ISerializedDataDestination&&) =
        delete;
    virtual ~ISerializedDataDestination() = default;

   protected:
    ISerializedDataDestination() = default;
};
}  // namespace launchdarkly::server_side::data_system
