#pragma once

#include <launchdarkly/server_side/integrations/iserialized_item_kind.hpp>
#include <launchdarkly/server_side/integrations/serialized_item_descriptor.hpp>

#include <tl/expected.hpp>

#include <optional>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::data_interfaces {

/**
 * Interface for a data reader that provides feature flags and related data in a
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
class ISerializedDataReader {
   public:
    virtual ~ISerializedDataReader() = default;
    ISerializedDataReader(ISerializedDataReader const& item) = delete;
    ISerializedDataReader(ISerializedDataReader&& item) = delete;
    ISerializedDataReader& operator=(ISerializedDataReader const&) = delete;
    ISerializedDataReader& operator=(ISerializedDataReader&&) = delete;

    struct Error {
        std::string message;
    };

    using GetResult =
        tl::expected<integrations::SerializedItemDescriptor, Error>;

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
    virtual GetResult Get(integrations::ISerializedItemKind const& kind,
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
    virtual AllResult All(
        integrations::ISerializedItemKind const& kind) const = 0;

    /**
     * @return Identity of the reader. Used in logs.
     */
    virtual std::string const& Identity() const = 0;

    /**
     * @return True if the reader has data that can be queried. The reader
     * should derive this state externally; that is, it should be an attribute
     * of the underlying source of data (not in memory.) A possible
     * implementation would be to store a special data key that is only set
     * after initial SDK data is stored.
     */
    virtual bool Initialized() const = 0;

   protected:
    ISerializedDataReader() = default;
};
}  // namespace launchdarkly::server_side::data_interfaces
