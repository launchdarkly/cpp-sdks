#pragma once

#include "../../../include/launchdarkly/server_side/integrations/data_reader/kinds.hpp"
#include "../../data_interfaces/source/idata_reader.hpp"

#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/serialization/value_mapping.hpp>
#include <launchdarkly/server_side/integrations/data_reader/iserialized_data_reader.hpp>

#include <memory>

namespace launchdarkly {

tl::expected<std::optional<data_model::Tombstone>, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<std::optional<data_model::Tombstone>,
                                           JsonError>> const& unused,
    boost::json::value const& json_value);
}  // namespace launchdarkly

namespace launchdarkly::server_side::data_components {

class JsonDeserializer final : public data_interfaces::IDataReader {
   public:
    explicit JsonDeserializer(
        Logger const& logger,
        std::shared_ptr<integrations::ISerializedDataReader> reader);

    [[nodiscard]] SingleResult<data_model::Flag> GetFlag(
        std::string const& key) const override;

    [[nodiscard]] SingleResult<data_model::Segment> GetSegment(
        std::string const& key) const override;

    [[nodiscard]] CollectionResult<data_model::Flag> AllFlags() const override;

    [[nodiscard]] CollectionResult<data_model::Segment> AllSegments()
        const override;

    [[nodiscard]] std::string const& Identity() const override;

    [[nodiscard]] bool Initialized() const override;

   private:
    template <typename Item>
    static tl::expected<data_model::ItemDescriptor<Item>,
                        data_interfaces::IDataReader::Error>
    DeserializeJsonDescriptor(
        integrations::SerializedItemDescriptor const& descriptor) {
        if (descriptor.deleted) {
            return data_model::ItemDescriptor<Item>(
                data_model::Tombstone(descriptor.version));
        }

        auto const json_val = boost::json::parse(descriptor.serializedItem);

        if (auto item_result = boost::json::value_to<
                tl::expected<std::optional<Item>, JsonError>>(json_val)) {
            auto item = *item_result;
            if (!item) {
                return tl::make_unexpected("item invalid: value is null");
            }
            return data_model::ItemDescriptor<Item>(std::move(*item));
        }

        auto tombstone = boost::json::value_to<
            tl::expected<std::optional<data_model::Tombstone>, JsonError>>(
            json_val);
        if (!tombstone) {
            return tl::make_unexpected(ErrorToString(tombstone.error()));
        }
        auto tombstone_result = *tombstone;
        if (!tombstone_result) {
            return tl::make_unexpected("tombstone invalid: value is null");
        }
        return data_model::ItemDescriptor<Item>(*tombstone_result);
    }

    template <typename DataModel, typename DataKind>
    SingleResult<DataModel> DeserializeSingle(DataKind const& kind,
                                              std::string const& key) const {
        auto result = source_->Get(kind, key);

        if (!result) {
            /* error in fetching the item */
            return tl::make_unexpected(result.error().message);
        }

        auto serialized_item = *result;

        if (!serialized_item) {
            return std::nullopt;
        }

        return DeserializeJsonDescriptor<DataModel>(*serialized_item);
    }

    template <typename DataModel, typename DataKind>
    CollectionResult<DataModel> DeserializeCollection(
        DataKind const& kind) const {
        auto result = source_->All(kind);

        if (!result) {
            /* error in fetching the items */
            return tl::make_unexpected(result.error().message);
        }

        Collection<DataModel> items;

        for (auto const& [key, descriptor] : *result) {
            auto item = DeserializeJsonDescriptor<DataModel>(descriptor);

            if (!item) {
                LD_LOG(logger_, LogLevel::kError)
                    << "failed to deserialize " << key << " while fetching all "
                    << kind.Namespace() << ": " << item.error();
                continue;
            }

            items.emplace(key, *item);
        }
        return items;
    }

    Logger const& logger_;
    integrations::FlagKind const flag_kind_;
    integrations::SegmentKind const segment_kind_;
    std::shared_ptr<integrations::ISerializedDataReader> source_;
    std::string const identity_;
};

}  // namespace launchdarkly::server_side::data_components
