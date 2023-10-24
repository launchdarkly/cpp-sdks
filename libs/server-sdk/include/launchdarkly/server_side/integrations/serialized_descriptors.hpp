#pragma once

#include <optional>
#include <string>

namespace launchdarkly::server_side::integrations {

/**
 * A versioned item which can be stored in a persistent store.
 */
struct SerializedItemDescriptor {
    std::uint64_t version;

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
    [[nodiscard]] virtual std::string const& Namespace() const = 0;

    /**
     * Deserialize data and return the version of the data.
     *
     * This is for cases where the persistent store cannot avoid deserializing
     * data to determine its version. For instance a Redis store where
     * the only columns are the prefixed key and the serialized data.
     *
     * If the data cannot be deserialized, then 0 will be returned.
     *
     * @param data The data to deserialize.
     * @return The version of the data.
     */
    [[nodiscard]] virtual uint64_t Version(std::string const& data) const = 0;

    IPersistentKind(IPersistentKind const& item) = delete;
    IPersistentKind(IPersistentKind&& item) = delete;
    IPersistentKind& operator=(IPersistentKind const&) = delete;
    IPersistentKind& operator=(IPersistentKind&&) = delete;
    virtual ~IPersistentKind() = default;

   protected:
    IPersistentKind() = default;
};

// TODO Find out where to put these

template <typename TData>
static uint64_t GetVersion(std::string data) {
    boost::json::error_code error_code;
    auto parsed = boost::json::parse(data, error_code);

    if (error_code) {
        return 0;
    }
    auto res =
        boost::json::value_to<tl::expected<std::optional<TData>, JsonError>>(
            parsed);

    if (res.has_value() && res->has_value()) {
        return res->value().version;
    }
    return 0;
}

std::string const& PersistentStore::SegmentKind::Namespace() const {
    return namespace_;
}

uint64_t PersistentStore::SegmentKind::Version(std::string const& data) const {
    return GetVersion<data_model::Segment>(data);
}

std::string const& PersistentStore::FlagKind::Namespace() const {
    return namespace_;
}

uint64_t PersistentStore::FlagKind::Version(std::string const& data) const {
    return GetVersion<data_model::Flag>(data);
}

}  // namespace launchdarkly::server_side::integrations
