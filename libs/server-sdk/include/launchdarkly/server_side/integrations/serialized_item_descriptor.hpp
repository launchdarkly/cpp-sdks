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

    static SerializedItemDescriptor Present(std::uint64_t version,
                                            std::string data) {
        return SerializedItemDescriptor{version, false, std::move(data)};
    }

    static SerializedItemDescriptor Absent(std::uint64_t const version,
                                           std::string tombstone_rep) {
        return SerializedItemDescriptor{version, true,
                                        std::move(tombstone_rep)};
    }
};

}  // namespace launchdarkly::server_side::integrations
