#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

#include <tl/expected.hpp>

namespace launchdarkly::server_side::integrations {

/**
 * A versioned item which can be stored in a persistent store.
 */
struct SerializedItemDescriptor {
    uint64_t version;

    /**
     * During an Init/Upsert, when this is true, the serializedItem will
     * contain a tombstone representation. If the persistence implementation
     * can efficiently store the deletion state, and version, then it may
     * choose to discard the item.
     */
    bool deleted;

    /**
     * When reading from a persistent store the serializedItem may be
     * std::nullopt for deleted items.
     */
    std::optional<std::string> serializedItem;
};

/**
 * Represents a namespace of persistent data.
 */
class IPersistentKind {
   public:
    /**
     * The namespace for the data.
     */
    [[nodiscard]] virtual std::string const& Namespace();

    /**
     * Deserialize data and return the version of the data.
     *
     * This is for cases where the persistent store cannot avoid deserializing
     * data to determine its version. For instance a Redis store where
     * the only columns are the prefixed key and the serialized data.
     *
     * @param data The data to deserialize.
     * @return The version of the data.
     */
    [[nodiscard]] virtual uint64_t Version(std::string const& data);

    IPersistentKind(IPersistentKind const& item) = delete;
    IPersistentKind(IPersistentKind&& item) = delete;
    IPersistentKind& operator=(IPersistentKind const&) = delete;
    IPersistentKind& operator=(IPersistentKind&&) = delete;
    virtual ~IPersistentKind() = default;

   protected:
    IPersistentKind() = default;
};

/**
 * Interface for a data store that holds feature flags and related data in a
 * serialized form.
 *
 * This interface should be used for database integrations, or any other data
 * store implementation that stores data in some external service.
 * The SDK will take care of converting between its own internal data model and
 * a serialized string form; the data store interacts only with the serialized
 * form.
 *
 * The SDK will also provide its own caching layer on top of the persistent data
 * store; the data store implementation should not provide caching, but simply
 * do every query or update that the SDK tells it to do.
 *
 * Implementations must be thread-safe.
 */
class IPersistentStoreCore {
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

    struct Error {
        std::string message;
    };

    using GetResult =
        tl::expected<std::optional<SerializedItemDescriptor>, Error>;

    using AllResult =
        tl::expected<std::unordered_map<std::string, SerializedItemDescriptor>,
                     Error>;

    using ItemKey = std::string;
    using KeyItemPair = std::pair<ItemKey, SerializedItemDescriptor>;
    using OrderedNamepace = std::vector<KeyItemPair>;
    using KindCollectionPair =
        std::pair<IPersistentKind const&, OrderedNamepace>;
    using OrderedData = std::vector<KindCollectionPair>;

    /**
     * Overwrites the store's contents with a set of items for each collection.
     *
     * All previous data should be discarded, regardless of versioning.
     *
     * The update should be done atomically. If it cannot be done atomically,
     * then the store must first add or update each item in the same order that
     * they are given in the input data, and then delete any previously stored
     * items that were not in the input data.
     *
     * @param allData The ordered set of data to replace all current data with.
     * @return The status of the init operation.
     */
    virtual InitResult Init(OrderedData const& allData) = 0;

    /**
     * Updates or inserts an item in the specified collection. For updates, the
     * object will only be updated if the existing version is less than the new
     * version.
     *
     * @param kind The collection kind to use.
     * @param itemKey The unique key for the item within the collection.
     * @param item The item to insert or update.
     *
     * @return The status of the operation.
     */
    virtual UpsertResult Upsert(IPersistentKind const& kind,
                                std::string const& itemKey,
                                SerializedItemDescriptor const& item) = 0;

    /**
     * Retrieves an item from the specified collection, if available.
     *
     * @param kind The kind of the item.
     * @param itemKey The key for the item.
     * @return A serialized item descriptor if the item existed, a std::nullopt
     * if the item did not exist, or an error. For a deleted item the serialized
     * item descriptor may contain a std::nullopt for the serializedItem.
     */
    virtual GetResult Get(IPersistentKind const& kind,
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
    virtual AllResult All(IPersistentKind const& kind) const = 0;

    /**
     * Returns true if this store has been initialized.
     *
     * In a shared data store, the implementation should be able to detect this
     * state even if Init was called in a different process, i.e. it must query
     * the underlying data store in some way. The method does not need to worry
     * about caching this value; the SDK will call it rarely.
     *
     * @return True if the store has been initialized.
     */
    virtual bool Initialized() const = 0;

    /**
     * A short description of the store, for instance "Redis". May be used
     * in diagnostic information and logging.
     *
     * @return A short description of the sore.
     */
    virtual std::string const& Description() const = 0;

    IPersistentStoreCore(IPersistentStoreCore const& item) = delete;
    IPersistentStoreCore(IPersistentStoreCore&& item) = delete;
    IPersistentStoreCore& operator=(IPersistentStoreCore const&) = delete;
    IPersistentStoreCore& operator=(IPersistentStoreCore&&) = delete;
    virtual ~IPersistentStoreCore() = default;

   protected:
    IPersistentStoreCore() = default;
};
}  // namespace launchdarkly::server_side::integrations
