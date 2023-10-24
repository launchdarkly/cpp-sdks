#pragma once

#include <launchdarkly/server_side/integrations/serialized_descriptors.hpp>

#include <launchdarkly/data_model/descriptors.hpp>
#include <launchdarkly/data_model/sdk_data_set.hpp>

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

    using ItemKey = std::string;
    using KeyItemPair =
        std::pair<ItemKey, integrations::SerializedItemDescriptor>;
    using OrderedNamepace = std::vector<KeyItemPair>;
    using KindCollectionPair =
        std::pair<integrations::IPersistentKind const&, OrderedNamepace>;
    using OrderedData = std::vector<KindCollectionPair>;

    virtual InitResult Init(OrderedData sdk_data_set) = 0;

    virtual UpsertResult Upsert(
        std::string const& kind,
        std::string const& key,
        integrations::SerializedItemDescriptor item) = 0;

    virtual std::string Identity() const = 0;

    ISerializedDestination(ISerializedDestination const& item) = delete;
    ISerializedDestination(ISerializedDestination&& item) = delete;
    ISerializedDestination& operator=(ISerializedDestination const&) = delete;
    ISerializedDestination& operator=(ISerializedDestination&&) = delete;
    virtual ~ISerializedDestination() = default;

   protected:
    ISerializedDestination() = default;
};
}  // namespace launchdarkly::server_side::data_interfaces
