#pragma once

#include "../../data_interfaces/source/idata_reader.hpp"
#include "../kinds/kinds.hpp"

#include <launchdarkly/server_side/data_interfaces/sources/iserialized_data_reader.hpp>

namespace launchdarkly::server_side::data_components {

class JsonDataReader final : public data_interfaces::IDataReader {
   public:
    explicit JsonDataReader(data_interfaces::ISerializedDataReader& reader);

    [[nodiscard]] Single<data_model::FlagDescriptor> GetFlag(
        std::string const& key) const override;

    [[nodiscard]] Single<data_model::SegmentDescriptor> GetSegment(
        std::string const& key) const override;

    [[nodiscard]] Collection<data_model::FlagDescriptor> AllFlags()
        const override;

    [[nodiscard]] Collection<data_model::SegmentDescriptor> AllSegments()
        const override;

    [[nodiscard]] std::string const& Identity() const override;

   private:
    template <typename DataModel, typename DataKind>
    Single<data_model::ItemDescriptor<DataModel>> Deserialize(
        DataKind const& kind,
        std::string const& key) const {
        auto result = source_.Get(kind, key);

        if (!result) {
            /* the actual fetch failed */
            return tl::make_unexpected(result.error().message);
        }

        if (!result->serializedItem) {
            /* the fetch succeeded, but the item wasn't found */
            return std::nullopt;
        }

        auto const boost_json_val = boost::json::parse(*result->serializedItem);
        auto flag = boost::json::value_to<
            tl::expected<std::optional<DataModel>, JsonError>>(boost_json_val);

        if (!flag) {
            /* flag couldn't be deserialized from the JSON string */
            return tl::make_unexpected(ErrorToString(flag.error()));
        }

        std::optional<DataModel> maybe_flag = flag->value();

        if (!maybe_flag) {
            /* JSON was valid, but the value is 'null'
             * TODO: will this ever happen?
             */
            return tl::make_unexpected("data was null");
        }

        return data_model::ItemDescriptor<DataModel>(std::move(*maybe_flag));
    }

    FlagKind const flag_kind_;
    FlagKind const segment_kind_;
    data_interfaces::ISerializedDataReader& reader_;
};

}  // namespace launchdarkly::server_side::data_components
