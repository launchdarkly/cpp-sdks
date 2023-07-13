#include "data_source_event_handler.hpp"

#include <launchdarkly/encoding/base_64.hpp>
#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_primitives.hpp>
#include <launchdarkly/serialization/json_rule_clause.hpp>
#include <launchdarkly/serialization/json_sdk_data_set.hpp>
#include <launchdarkly/serialization/json_segment.hpp>
#include <launchdarkly/serialization/value_mapping.hpp>

#include <boost/core/ignore_unused.hpp>
#include <boost/json.hpp>
#include <unordered_map>

#include <utility>

#include "tl/expected.hpp"

namespace launchdarkly::server_side::data_sources {

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

tl::expected<DataSourceEventHandler::Patch, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<DataSourceEventHandler::Patch, JsonError>> const& unused,
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
    } else if (StreamingDataKinds::Segment::IsKind(path)) {
        return Patch<StreamingDataKinds::Segment, data_model::Segment>(path,
                                                                       obj);
    }

    // TODO: Implement
    return tl::unexpected(JsonError::kSchemaFailure);
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
    auto const* path_iter = obj.find("path");
    auto path = ValueAsOpt<std::string>(path_iter, obj.end());
    auto const* version_iter = obj.find("version");
    std::optional<uint64_t> version =
        ValueAsOpt<uint64_t>(version_iter, obj.end());

    if (path.has_value() && version.has_value()) {
        auto kind = StreamingDataKinds::Kind(*path);
        auto key = StreamingDataKinds::Key(*path);

        if (kind.has_value() && key.has_value()) {
            return DataSourceEventHandler::Delete{*key, *kind, *version};
        }
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    return tl::unexpected(JsonError::kSchemaFailure);
}

DataSourceEventHandler::DataSourceEventHandler(
    IDataSourceUpdateSink& handler,
    Logger const& logger,
    DataSourceStatusManager& status_manager)
    : handler_(handler), logger_(logger), status_manager_(status_manager) {}

DataSourceEventHandler::MessageStatus DataSourceEventHandler::HandleMessage(
    std::string const& type,
    std::string const& data) {
    if (type == "put") {
        boost::json::error_code error_code;
        auto parsed = boost::json::parse(data, error_code);
        if (error_code) {
            LD_LOG(logger_, LogLevel::kError) << kErrorParsingPut;
            status_manager_.SetError(
                DataSourceStatus::ErrorInfo::ErrorKind::kInvalidData,
                kErrorParsingPut);
            return DataSourceEventHandler::MessageStatus::kInvalidMessage;
        }
        auto res = boost::json::value_to<
            tl::expected<data_model::SDKDataSet, JsonError>>(parsed);

        if (res.has_value()) {
            handler_.Init(std::move(*res));
            status_manager_.SetState(DataSourceStatus::DataSourceState::kValid);
            return DataSourceEventHandler::MessageStatus::kMessageHandled;
        }
        LD_LOG(logger_, LogLevel::kError) << kErrorPutInvalid;
        status_manager_.SetError(
            DataSourceStatus::ErrorInfo::ErrorKind::kInvalidData,
            kErrorPutInvalid);
        return DataSourceEventHandler::MessageStatus::kInvalidMessage;
    }
    if (type == "patch") {
        boost::json::error_code error_code;
        auto parsed = boost::json::parse(data, error_code);
        if (error_code) {
            LD_LOG(logger_, LogLevel::kError) << kErrorParsingPut;
            status_manager_.SetError(
                DataSourceStatus::ErrorInfo::ErrorKind::kInvalidData,
                kErrorParsingPatch);
            return DataSourceEventHandler::MessageStatus::kInvalidMessage;
        }

        auto res = boost::json::value_to<
            tl::expected<DataSourceEventHandler::Patch, JsonError>>(parsed);

        if (res.has_value()) {
            auto const& key = res->key;
            std::visit([this, &key](auto&& arg) { handler_.Upsert(key, arg); },
                       res->data);
            return DataSourceEventHandler::MessageStatus::kMessageHandled;
        }

        status_manager_.SetError(
            DataSourceStatus::ErrorInfo::ErrorKind::kInvalidData,
            kErrorPatchInvalid);
        return DataSourceEventHandler::MessageStatus::kInvalidMessage;
    }
    if (type == "delete") {
        boost::json::error_code error_code;
        auto parsed = boost::json::parse(data, error_code);
        if (error_code) {
            LD_LOG(logger_, LogLevel::kError) << kErrorParsingDelete;
            status_manager_.SetError(
                DataSourceStatus::ErrorInfo::ErrorKind::kInvalidData,
                kErrorParsingDelete);
            return DataSourceEventHandler::MessageStatus::kInvalidMessage;
        }

        auto res = boost::json::value_to<
            tl::expected<DataSourceEventHandler::Delete, JsonError>>(parsed);

        if (res.has_value()) {
            switch (res->kind) {
                case data_store::DataKind::kFlag: {
                    handler_.Upsert(res->key,
                                    data_store::FlagDescriptor(res->version));
                    return DataSourceEventHandler::MessageStatus::
                        kMessageHandled;
                }
                case data_store::DataKind::kSegment: {
                    handler_.Upsert(
                        res->key, data_store::SegmentDescriptor(res->version));
                    return DataSourceEventHandler::MessageStatus::
                        kMessageHandled;
                }
                default: {
                } break;
            }
        }

        status_manager_.SetError(
            DataSourceStatus::ErrorInfo::ErrorKind::kInvalidData,
            kErrorPatchInvalid);
        return DataSourceEventHandler::MessageStatus::kInvalidMessage;
    }
    status_manager_.SetError(
        DataSourceStatus::ErrorInfo::ErrorKind::kInvalidData,
        kErrorPatchInvalid);
    return DataSourceEventHandler::MessageStatus::kUnhandledVerb;
}

// DataSourceEventHandler::StreamingDataKind::StreamingDataKind(
//     data_store::DataKind kind,
//     std::string path)
//     : kind_(kind), path_(std::move(path)) {}
//
// data_store::DataKind DataSourceEventHandler::StreamingDataKind::Kind() const
// {
//     return kind_;
// }
//
// std::string const& DataSourceEventHandler::StreamingDataKind::Path() const {
//     return path_;
// }

}  // namespace launchdarkly::server_side::data_sources
