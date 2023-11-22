#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace launchdarkly::server_side::integrations {
/**
 * A versioned item which can be stored or loaded from a persistent store.
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

    /**
     * @brief Constructs a SerializedItemDescriptor from a version and a
     * serialized item.
     * @param version Version of item.
     * @param data Serialized item.
     * @return SerializedItemDescriptor.
     */
    static SerializedItemDescriptor Present(std::uint64_t version,
                                            std::string data) {
        return SerializedItemDescriptor{version, false, std::move(data)};
    }

    /**
     * @brief Constructs a SerializedItemDescriptor from a version and a
     * tombstone.
     *
     * This is used when an item is deleted: the tombstone can be stored in
     * place of the item, and the version checked in the future. Without the
     * tombstone, out-of-order data updates could "resurrect" a deleted item.
     *
     * @param version Version of the item.
     * @param tombstone_rep Serialized tombstone representation of the item.
     * @return SerializedItemDescriptor.
     */
    static SerializedItemDescriptor Absent(std::uint64_t const version,
                                           std::string tombstone_rep) {
        return SerializedItemDescriptor{version, true,
                                        std::move(tombstone_rep)};
    }
}
};

}  // namespace launchdarkly::server_side::integrations
