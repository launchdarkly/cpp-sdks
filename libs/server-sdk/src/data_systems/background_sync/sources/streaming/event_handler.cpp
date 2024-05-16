#include "event_handler.hpp"

#include <launchdarkly/encoding/base_64.hpp>
#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_sdk_data_set.hpp>
#include <launchdarkly/serialization/json_segment.hpp>
#include <launchdarkly/serialization/value_mapping.hpp>

#include <boost/core/ignore_unused.hpp>
#include <boost/json.hpp>

#include <optional>
#include <unordered_map>
#include <utility>

#include <tl/expected.hpp>

namespace launchdarkly::server_side::data_systems {

static char const* const kErrorParsingPut = "Could not parse PUT message";
static char const* const kErrorPutInvalid =
    "PUT message contained invalid data";
static char const* const kErrorParsingPatch = "Could not parse PATCH message";
static char const* const kErrorPatchInvalid =
    "PATCH message contained invalid data";
static char const* const kErrorParsingDelete = "Could not parse DELETE message";
static char const* const kErrorDeleteInvalid =
    "DELETE message contained invalid data\"";

template <typename TStreamingDataKind, typename TData>
tl::expected<DataSourceEventHandler::Patch, JsonError> Patch(
    std::string const& path,
    boost::json::object const& obj) {
    auto const* data_iter = obj.find("data");
    if (data_iter == obj.end()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    auto data =
        boost::json::value_to<tl::expected<std::optional<TData>, JsonError>>(
            data_iter->value());
    if (!data.has_value()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    return DataSourceEventHandler::Patch{
        TStreamingDataKind::Key(path),
        data_model::ItemDescriptor<TData>(data->value())};
}

tl::expected<std::optional<DataSourceEventHandler::Put>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<DataSourceEventHandler::Put>,
                     JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    if (!json_value.is_object()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }

    DataSourceEventHandler::Put put;
    std::string path;
    auto const& obj = json_value.as_object();
    PARSE_FIELD(path, obj, "path");
    // We don't know what to do with a path other than "/".
    if (!(path == "/" || path.empty())) {
        return std::nullopt;
    }
    PARSE_FIELD(put.data, obj, "data");

    return put;
}

tl::expected<std::optional<DataSourceEventHandler::Patch>, JsonError>
tag_invoke(boost::json::value_to_tag<
               tl::expected<std::optional<DataSourceEventHandler::Patch>,
                            JsonError>> const& unused,
           boost::json::value const& json_value) {
    boost::ignore_unused(unused);
    if (!json_value.is_object()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }

    auto const& obj = json_value.as_object();

    std::string path;
    PARSE_FIELD(path, obj, "path");

    if (StreamingDataKinds::Flag::IsKind(path)) {
        return Patch<StreamingDataKinds::Flag, data_model::Flag>(path, obj);
    }

    if (StreamingDataKinds::Segment::IsKind(path)) {
        return Patch<StreamingDataKinds::Segment, data_model::Segment>(path,
                                                                       obj);
    }

    return std::nullopt;
}

static tl::expected<DataSourceEventHandler::Delete, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<DataSourceEventHandler::Delete, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    if (!json_value.is_object()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }

    auto const& obj = json_value.as_object();

    DataSourceEventHandler::Delete del;
    PARSE_REQUIRED_FIELD(del.version, obj, "version");
    std::string path;
    PARSE_REQUIRED_FIELD(path, obj, "path");

    auto kind = StreamingDataKinds::Kind(path);
    auto key = StreamingDataKinds::Key(path);

    if (kind.has_value() && key.has_value()) {
        del.kind = *kind;
        del.key = *key;
        return del;
    }
    return tl::unexpected(JsonError::kSchemaFailure);
}

DataSourceEventHandler::DataSourceEventHandler(
    data_interfaces::IDestination& handler,
    Logger const& logger,
    data_components::DataSourceStatusManager& status_manager)
    : handler_(handler), logger_(logger), status_manager_(status_manager) {}

DataSourceEventHandler::MessageStatus DataSourceEventHandler::HandleMessage(
    std::string const& type,
    std::string const& data) {
    if (type == "put") {
        boost::system::error_code error_code;
        auto parsed = boost::json::parse(data, error_code);
        if (error_code) {
            LD_LOG(logger_, LogLevel::kError) << kErrorParsingPut;
            status_manager_.SetError(
                DataSourceStatus::ErrorInfo::ErrorKind::kInvalidData,
                kErrorParsingPut);
            return MessageStatus::kInvalidMessage;
        }
        auto res =
            boost::json::value_to<tl::expected<std::optional<Put>, JsonError>>(
                parsed);

        if (!res) {
            LD_LOG(logger_, LogLevel::kError) << kErrorPutInvalid;
            status_manager_.SetError(
                DataSourceStatus::ErrorInfo::ErrorKind::kInvalidData,
                kErrorPutInvalid);
            return MessageStatus::kInvalidMessage;
        }

        // Check the inner optional.
        if (res->has_value()) {
            handler_.Init(std::move((*res)->data));
            status_manager_.SetState(DataSourceStatus::DataSourceState::kValid);
            return MessageStatus::kMessageHandled;
        }
        return MessageStatus::kMessageHandled;
    }
    if (type == "patch") {
        boost::system::error_code error_code;
        auto parsed = boost::json::parse(data, error_code);
        if (error_code) {
            LD_LOG(logger_, LogLevel::kError) << kErrorParsingPut;
            status_manager_.SetError(
                DataSourceStatus::ErrorInfo::ErrorKind::kInvalidData,
                kErrorParsingPatch);
            return MessageStatus::kInvalidMessage;
        }

        auto res = boost::json::value_to<
            tl::expected<std::optional<Patch>, JsonError>>(parsed);

        if (!res.has_value()) {
            status_manager_.SetError(
                DataSourceStatus::ErrorInfo::ErrorKind::kInvalidData,
                kErrorPatchInvalid);
            return MessageStatus::kInvalidMessage;
        }

        // This references the optional inside the expected.
        if (res->has_value()) {
            auto const& patch = (**res);
            auto const& key = patch.key;
            std::visit([this, &key](auto&& arg) { handler_.Upsert(key, arg); },
                       patch.data);
            return MessageStatus::kMessageHandled;
        }
        // We didn't recognize the type of the patch. So we ignore it.
        return MessageStatus::kMessageHandled;
    }
    if (type == "delete") {
        boost::system::error_code error_code;
        auto parsed = boost::json::parse(data, error_code);
        if (error_code) {
            LD_LOG(logger_, LogLevel::kError) << kErrorParsingDelete;
            status_manager_.SetError(
                DataSourceStatus::ErrorInfo::ErrorKind::kInvalidData,
                kErrorParsingDelete);
            return MessageStatus::kInvalidMessage;
        }

        auto res =
            boost::json::value_to<tl::expected<Delete, JsonError>>(parsed);

        if (res.has_value()) {
            switch (res->kind) {
                case data_components::DataKind::kFlag: {
                    handler_.Upsert(res->key,
                                    data_model::FlagDescriptor(
                                        data_model::Tombstone(res->version)));
                    return MessageStatus::kMessageHandled;
                }
                case data_components::DataKind::kSegment: {
                    handler_.Upsert(res->key,
                                    data_model::SegmentDescriptor(
                                        data_model::Tombstone(res->version)));
                    return MessageStatus::kMessageHandled;
                }
                default: {
                } break;
            }
        }

        status_manager_.SetError(
            DataSourceStatus::ErrorInfo::ErrorKind::kInvalidData,
            kErrorDeleteInvalid);
        return MessageStatus::kInvalidMessage;
    }

    return MessageStatus::kUnhandledVerb;
}

}  // namespace launchdarkly::server_side::data_systems
