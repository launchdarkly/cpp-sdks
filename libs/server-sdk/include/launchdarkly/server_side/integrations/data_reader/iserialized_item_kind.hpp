#pragma once

#include <cstdint>
#include <ostream>
#include <string>

namespace launchdarkly::server_side::integrations {

/**
 * \brief Represents the kind of a serialized item. The purpose of this
 * interface is to allow for determining a serialized item's version without
 * leaking the details of the serialization format to the calling code.
 */
class ISerializedItemKind {
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
    [[nodiscard]] virtual std::uint64_t Version(
        std::string const& data) const = 0;

    ISerializedItemKind(ISerializedItemKind const& item) = delete;
    ISerializedItemKind(ISerializedItemKind&& item) = delete;
    ISerializedItemKind& operator=(ISerializedItemKind const&) = delete;
    ISerializedItemKind& operator=(ISerializedItemKind&&) = delete;
    virtual ~ISerializedItemKind() = default;

   protected:
    ISerializedItemKind() = default;
};

/**
 * @brief Used in test assertions; two ISerializedItemKinds are regarded as
 * the same if they have the same namespace.
 */
inline bool operator==(ISerializedItemKind const& lhs,
                       ISerializedItemKind const& rhs) {
    return lhs.Namespace() == rhs.Namespace();
}

inline void PrintTo(ISerializedItemKind const& kind, std::ostream* os) {
    *os << kind.Namespace();
}

}  // namespace launchdarkly::server_side::integrations
