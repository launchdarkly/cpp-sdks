#pragma once

#include "../../data_components/kinds/kinds.hpp"
#include "../../data_interfaces/destination/idestination.hpp"
#include "../../data_interfaces/destination/iserialized_destination.hpp"

namespace launchdarkly::server_side::data_components {

/**
 * @brief JsonDestination is responsible for converting flag and segment
 * models into serialized data suitable for storage in an
 * ISerializedDestination.
 *
 * It's purpose is to encapsulate the details of serialization in a reusable
 * adapter.
 *
 * JsonDestination does not initialize ISerializedDestination with a
 * flag-dependency-order data layout, which is required for some stores (e.g.
 * DynamoDB).
 *
 * Since DynamoDB is not supported at the moment this is acceptable. When the
 * capability is needed, a new class should be derived from JsonDestination
 * overriding Init to provide the correct data layout.
 *
 * Alternatively, JsonDestination can be made to order the data so that it works
 * for any store.
 *
 */
class JsonDestination : public data_interfaces::IDestination {
   public:
    /**
     * @brief Construct the JsonDestination with the given destination.
     * Calls to Upsert will trigger serialization and storage in the
     * destination.
     * @param destination Where data should be forwarded.
     */
    explicit JsonDestination(
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

   private:
    data_interfaces::ISerializedDestination& dest_;
    struct Kinds {
        static FlagKind const Flag;
        static SegmentKind const Segment;
    };
};

}  // namespace launchdarkly::server_side::data_components
