#pragma once

#include "../../data_interfaces/source/idata_reader.hpp"
#include "../kinds/kinds.hpp"

#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/server_side/data_interfaces/sources/iserialized_data_reader.hpp>

#include <memory>

namespace launchdarkly::server_side::data_components {

class JsonDeserializer final : public data_interfaces::IDataReader {
   public:
    explicit JsonDeserializer(
        Logger const& logger,
        std::shared_ptr<data_interfaces::ISerializedDataReader> reader);

    [[nodiscard]] Single<data_model::FlagDescriptor> GetFlag(
        std::string const& key) const override;

    [[nodiscard]] Single<data_model::SegmentDescriptor> GetSegment(
        std::string const& key) const override;

    [[nodiscard]] Collection<data_model::FlagDescriptor> AllFlags()
        const override;

    [[nodiscard]] Collection<data_model::SegmentDescriptor> AllSegments()
        const override;

    [[nodiscard]] std::string const& Identity() const override;

    [[nodiscard]] bool Initialized() const override;

   private:
    template <typename DataModel, typename DataKind>
    tl::expected<data_model::ItemDescriptor<DataModel>, std::string>
    DeserializeItem(std::string const& serialized_item) const {
        auto const boost_json_val = boost::json::parse(serialized_item);
        auto item = boost::json::value_to<
            tl::expected<std::optional<DataModel>, JsonError>>(boost_json_val);

        if (!item) {
            return tl::make_unexpected(ErrorToString(item.error()));
        }

        std::optional<DataModel> maybe_item = item->value();

        if (!maybe_item) {
            return tl::make_unexpected("JSON value is null");
        }

        return data_model::ItemDescriptor<DataModel>(std::move(*maybe_item));
    }

    template <typename DataModel, typename DataKind>
    Single<data_model::ItemDescriptor<DataModel>> DeserializeSingle(
        DataKind const& kind,
        std::string const& key) const {
        auto result = source_->Get(kind, key);

        if (!result) {
            /* error in fetching the item */
            return tl::make_unexpected(result.error().message);
        }

        if (!result->serializedItem) {
            /* no error, but item not found by the source */
            return std::nullopt;
        }

        return DeserializeItem<DataModel, DataKind>(*result->serializedItem);
    }

    template <typename DataModel, typename DataKind>
    Collection<data_model::ItemDescriptor<DataModel>> DeserializeCollection(
        DataKind const& kind) const {
        auto result = source_->All(kind);

        if (!result) {
            /* error in fetching the items */
            return tl::make_unexpected(result.error().message);
        }

        std::unordered_map<std::string, data_model::ItemDescriptor<DataModel>>
            items;

        for (auto const& [key, descriptor] : *result) {
            if (!descriptor.serializedItem) {
                /* item is deleted, add a tombstone to the result so that the
                 * caller can make a decision on what to do. */
                items.emplace(key, data_model::ItemDescriptor<DataModel>(
                                       descriptor.version));
                continue;
            }

            auto maybe_item = DeserializeItem<DataModel, DataKind>(
                *descriptor.serializedItem);

            if (!maybe_item) {
                /* single item failing to deserialize doesn't cause the
                 * whole operation to fail; other items may be valid. */
                LD_LOG(logger_, LogLevel::kError)
                    << "failed to deserialize " << key << " while fetching all "
                    << kind.Namespace() << ": " << maybe_item.error();
                continue;
            }

            items.emplace(key, *maybe_item);
        }
        return items;
    }

    Logger const& logger_;
    FlagKind const flag_kind_;
    SegmentKind const segment_kind_;
    std::shared_ptr<data_interfaces::ISerializedDataReader> source_;
    std::string const identity_;
};

}  // namespace launchdarkly::server_side::data_components
