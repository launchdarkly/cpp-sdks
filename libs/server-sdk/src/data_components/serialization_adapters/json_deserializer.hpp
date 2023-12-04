#pragma once

#include "../../../include/launchdarkly/server_side/integrations/data_reader/kinds.hpp"
#include "../../data_interfaces/source/idata_reader.hpp"

#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/server_side/integrations/data_reader/iserialized_data_reader.hpp>

#include <memory>

namespace launchdarkly::server_side::data_components {

template <typename Item>
tl::expected<data_interfaces::IDataReader::StorageItem<Item>,
             data_interfaces::IDataReader::Error>
IntoStorageItem(integrations::SerializedItemDescriptor const& descriptor) {
    if (descriptor.deleted) {
        return data_interfaces::IDataReader::StorageItem<Item>(
            data_interfaces::IDataReader::Tombstone(descriptor.version));
    }

    auto const json_val = boost::json::parse(descriptor.serializedItem);

    auto result =
        boost::json::value_to<tl::expected<std::optional<Item>, JsonError>>(
            json_val);

    if (!result) {
        /* maybe it's a tombstone - check */
        /* TODO(225976): replace with boost::json deserializer */
        if (json_val.is_object()) {
            auto const& obj = json_val.as_object();
            if (auto deleted_it = obj.find("deleted");
                deleted_it != obj.end()) {
                auto const& deleted = deleted_it->value();

                if (deleted.is_bool() && deleted.as_bool()) {
                    if (auto version_it = obj.find("version");
                        version_it != obj.end()) {
                        auto const& version = version_it->value();
                        if (version.is_number()) {
                            return data_interfaces::IDataReader::Tombstone(
                                version.as_uint64());
                        }
                        return tl::make_unexpected(
                            data_interfaces::IDataReader::Error{
                                "tombstone field 'version' is invalid"});
                    }
                    return tl::make_unexpected(
                        "tombstone field 'version' is missing");
                }
                return tl::make_unexpected(
                    "tombstone field 'deleted' is invalid ");
            }
        }

        return tl::make_unexpected(
            "serialized item isn't a valid data item or tombstone");
    }

    auto item = *result;

    if (!item) {
        return tl::make_unexpected("serialized item is null JSON value");
    }

    return *item;
}

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

        return IntoStorageItem<DataModel>(*serialized_item);
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
            auto item = IntoStorageItem<DataModel>(descriptor);

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
