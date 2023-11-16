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
};

}  // namespace launchdarkly::server_side::integrations
