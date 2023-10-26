#pragma once

#include <launchdarkly/server_side/integrations/serialized_descriptors.hpp>

#include <tl/expected.hpp>

#include <optional>
#include <string>

namespace launchdarkly::server_side::data_interfaces {

/**
 * Interface for a data source that provides feature flags and related data in a
 * serialized form.
 *
 * This interface should be used for database integrations, or any other data
 * source implementation that retrieves data from some external service.
 *
 * The SDK will take care of converting between its own internal data model and
 * a serialized string form; the source interacts only with the serialized
 * form.
 *
 * The SDK will also provide its own caching layer in front of this source;
 * this source should not provide caching, but simply
 * do every query or update that the SDK tells it to do.
 *
 * Implementations must be thread-safe.
 */
class ISerializedDataPullSource {
   public:
    virtual ~ISerializedDataPullSource() = default;
    ISerializedDataPullSource(ISerializedDataPullSource const& item) = delete;
    ISerializedDataPullSource(ISerializedDataPullSource&& item) = delete;
    ISerializedDataPullSource& operator=(ISerializedDataPullSource const&) =
        delete;
    ISerializedDataPullSource& operator=(ISerializedDataPullSource&&) = delete;

    struct Error {
        std::string message;
    };

    using GetResult =
        tl::expected<std::optional<integrations::SerializedItemDescriptor>,
                     Error>;

    using AllResult = tl::expected<
        std::unordered_map<std::string, integrations::SerializedItemDescriptor>,
        Error>;

    /**
     * Retrieves an item from the specified collection, if available.
     *
     * @param kind The kind of the item.
     * @param itemKey The key for the item.
     * @return A serialized item descriptor if the item existed, a std::nullopt
     * if the item did not exist, or an error. For a deleted item the serialized
     * item descriptor may contain a std::nullopt for the serializedItem.
     */
    virtual GetResult Get(integrations::IPersistentKind const& kind,
                          std::string const& itemKey) const = 0;

    /**
     * Retrieves all items from the specified collection.
     *
     * If the store contains placeholders for deleted items, it should include
     * them in the results, not filter them out.
     * @param kind The kind of data to get.
     * @return Either all of the items of the type, or an error. If there are
     * no items of the specified type, then return an empty collection.
     */
    virtual AllResult All(integrations::IPersistentKind const& kind) const = 0;

    virtual std::string const& Identity() const = 0;

    virtual bool Initialized() const = 0;

   protected:
    ISerializedDataPullSource() = default;
};
}  // namespace launchdarkly::server_side::data_interfaces
