#pragma once

#include "../../data_components/kinds/kinds.hpp"
#include "../../data_interfaces/destination/idestination.hpp"
#include "../../data_interfaces/destination/iserialized_destination.hpp"

#include <launchdarkly/logging/logger.hpp>

namespace launchdarkly::server_side::data_components {

/**
 * @brief JsonDestination is responsible for converting flag and segment
 * models into serialized data suitable for storage in an
 * ISerializedDestination.
 *
 * By encapsulating the serialization logic here, different adapters can be
 * swapped in if our serialization format ever changes.
 *
 * JsonDestination does not currently initialize ISerializedDestination with a
 * flag-dependency-order payload, which is required to minimize bugs in
 * stores without atomic transactions (e.g. DynamoDB).
 *
 * Instead, it sorts items within a collection using 'operator<' on their keys,
 * giving which is enough determinism for testing purposes.
 *
 * TODO(sc-225327): Implement topographic sort as prerequisite for DynamoDB.
 *
 */
class JsonDestination final : public data_interfaces::IDestination {
   public:
    /**
     * @brief Construct the JsonDestination with the given
     * ISerializedDestination. Calls to Upsert will trigger serialization and
     * store to the destination.
     * @param logger Used for logging storage errors.
     * @param destination Where data should be forwarded.
     */
    explicit JsonDestination(
        Logger const& logger,
        data_interfaces::ISerializedDestination& destination);

    /**
     * @brief Initialize the destination with an SDK data set.
     * @param data_set The initial data.
     */
    void Init(data_model::SDKDataSet data_set) override;

    /**
     * @brief Upsert data for the flag named by key.
     *
     * If the descriptor represents a deleted item, a tombstone will
     * be forwarded to the ISerializedDestination.
     *
     * @param key Key of flag.
     * @param flag Descriptor of flag.
     */
    void Upsert(std::string const& key,
                data_model::FlagDescriptor flag) override;

    /**
     * @brief Upsert data for the segment named by key.
     *
     * If the descriptor represents a deleted item, a tombstone will
     * be forwarded to the ISerializedDestination.
     *
     * @param key Key of segment.
     * @param segment Descriptor of segment.
     */
    void Upsert(std::string const& key,
                data_model::SegmentDescriptor segment) override;

    /**
     * @return Identity of this destination. Used in logs.
     */
    [[nodiscard]] std::string const& Identity() const override;

    /**
     * @brief These are public so they can be referenced in tests.
     */
    struct Kinds {
        static FlagKind const Flag;
        static SegmentKind const Segment;
    };

   private:
    void LogUpsertResult(
        std::string const& key,
        std::string const& data_type,
        data_interfaces::ISerializedDestination::UpsertResult const& result)
        const;

    Logger const& logger_;
    data_interfaces::ISerializedDestination& dest_;
};

}  // namespace launchdarkly::server_side::data_components
