#pragma once

#include "../../data_interfaces/source/idata_reader.hpp"
#include "../kinds/kinds.hpp"

#include <launchdarkly/server_side/data_interfaces/sources/iserialized_data_reader.hpp>

#include <memory>

namespace launchdarkly::server_side::data_components {

class JsonDeserializer final : public data_interfaces::IDataReader {
   public:
    explicit JsonDeserializer(
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
            /* item couldn't be deserialized from the JSON string */
            return tl::make_unexpected(ErrorToString(item.error()));
        }

        std::optional<DataModel> maybe_item = item->value();

        if (!maybe_item) {
            /* JSON was valid, but the value is 'null'
             * TODO: log an error? Can this happen?
             */
            return tl::make_unexpected("data was null");
        }

        return data_model::ItemDescriptor<DataModel>(std::move(*maybe_item));
    }

    template <typename DataModel, typename DataKind>
    Single<data_model::ItemDescriptor<DataModel>> DeserializeSingle(
        DataKind const& kind,
        std::string const& key) const {
        auto result = reader_->Get(kind, key);

        if (!result) {
            /* the actual fetch failed */
            return tl::make_unexpected(result.error().message);
        }

        if (!result->serializedItem) {
            /* the fetch succeeded, but the item wasn't found */
            return std::nullopt;
        }

        return DeserializeItem<DataModel, DataKind>(*result->serializedItem);
    }

    template <typename DataModel, typename DataKind>
    Collection<data_model::ItemDescriptor<DataModel>> DeserializeCollection(
        DataKind const& kind) const {
        auto result = reader_->All(kind);

        if (!result) {
            /* the actual fetch failed */
            return tl::make_unexpected(result.error().message);
        }

        std::unordered_map<std::string, data_model::ItemDescriptor<DataModel>>
            items;

        for (auto const& [key, descriptor] : *result) {
            if (!descriptor.serializedItem) {
                // Item deleted
                continue;
            }

            auto maybe_item = DeserializeItem<DataModel, DataKind>(
                *descriptor.serializedItem);
            if (!maybe_item) {
                // TODO: Log the error?
                continue;
            }

            items.emplace(key, *maybe_item);
        }
        return items;
    }

    FlagKind const flag_kind_;
    FlagKind const segment_kind_;
    std::shared_ptr<data_interfaces::ISerializedDataReader> reader_;
};

}  // namespace launchdarkly::server_side::data_components
