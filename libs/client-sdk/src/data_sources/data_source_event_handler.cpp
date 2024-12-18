#include "data_source_event_handler.hpp"

#include <launchdarkly/encoding/base_64.hpp>
#include <launchdarkly/serialization/json_evaluation_result.hpp>
#include <launchdarkly/serialization/json_item_descriptor.hpp>
#include <launchdarkly/detail/serialization/json_primitives.hpp>
#include <launchdarkly/serialization/value_mapping.hpp>

#include <boost/core/ignore_unused.hpp>
#include <boost/json.hpp>
#include <unordered_map>

#include <utility>

#include "tl/expected.hpp"

namespace launchdarkly::client_side::data_sources {

static char const* const kErrorParsingPut = "Could not parse PUT message";
static char const* const kErrorPutInvalid =
    "PUT message contained invalid data";
static char const* const kErrorParsingPatch = "Could not parse PATCH message";
static char const* const kErrorPatchInvalid =
    "PATCH message contained invalid data";
static char const* const kErrorParsingDelete = "Could not parse DELETE message";
static char const* const kErrorDeleteInvalid =
    "DELETE message contained invalid data\"";

static tl::expected<DataSourceEventHandler::PatchData, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<DataSourceEventHandler::PatchData,
                                           JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    if (json_value.is_object()) {
        auto const& obj = json_value.as_object();
        auto const* key_iter = obj.find("key");
        auto key = ValueAsOpt<std::string>(key_iter, obj.end());
        auto result = boost::json::value_to<
            tl::expected<std::optional<EvaluationResult>, JsonError>>(
            json_value);

        if (result.has_value() && result.value().has_value() &&
            key.has_value()) {
            return DataSourceEventHandler::PatchData{key.value(),
                                                     result.value().value()};
        }
    }
    return tl::unexpected(JsonError::kSchemaFailure);
}

static tl::expected<DataSourceEventHandler::DeleteData, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<DataSourceEventHandler::DeleteData,
                                           JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    if (json_value.is_object()) {
        auto const& obj = json_value.as_object();
        auto const* key_iter = obj.find("key");
        auto key = ValueAsOpt<std::string>(key_iter, obj.end());
        auto const* version_iter = obj.find("version");
        std::optional<uint64_t> version =
            ValueAsOpt<uint64_t>(version_iter, obj.end());

        if (key.has_value() && version.has_value()) {
            return DataSourceEventHandler::DeleteData{key.value(),
                                                      version.value()};
        }
    }
    return tl::unexpected(JsonError::kSchemaFailure);
}

DataSourceEventHandler::DataSourceEventHandler(
    Context const& context,
    IDataSourceUpdateSink& handler,
    Logger const& logger,
    DataSourceStatusManager& status_manager)
    : context_(context),
      handler_(handler),
      logger_(logger),
      status_manager_(status_manager) {}

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
        auto res = boost::json::value_to<tl::expected<
            std::optional<std::unordered_map<std::string, ItemDescriptor>>,
            JsonError>>(parsed);

        if (res.has_value()) {
            // If the map was null or omitted, treat it like an empty data set.
            auto map = res.value().value_or(
                std::unordered_map<std::string, ItemDescriptor>{});

            handler_.Init(context_, std::move(map));
            status_manager_.SetState(DataSourceStatus::DataSourceState::kValid);
            return MessageStatus::kMessageHandled;
        }
        LD_LOG(logger_, LogLevel::kError) << kErrorPutInvalid;
        status_manager_.SetError(
            DataSourceStatus::ErrorInfo::ErrorKind::kInvalidData,
            kErrorPutInvalid);
        return MessageStatus::kInvalidMessage;
    }
    if (type == "patch") {
        boost::system::error_code error_code;
        auto parsed = boost::json::parse(data, error_code);
        if (error_code) {
            LD_LOG(logger_, LogLevel::kError) << kErrorParsingPatch;
            status_manager_.SetError(
                DataSourceStatus::ErrorInfo::ErrorKind::kInvalidData,
                kErrorParsingPatch);
            return MessageStatus::kInvalidMessage;
        }
        auto res =
            boost::json::value_to<tl::expected<PatchData, JsonError>>(parsed);
        if (res.has_value()) {
            handler_.Upsert(context_, res.value().key,
                            ItemDescriptor(res.value().flag));
            return MessageStatus::kMessageHandled;
        }
        LD_LOG(logger_, LogLevel::kError) << kErrorPatchInvalid;
        status_manager_.SetError(
            DataSourceStatus::ErrorInfo::ErrorKind::kInvalidData,
            kErrorPatchInvalid);
        return MessageStatus::kInvalidMessage;
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
        auto res = boost::json::value_to<tl::expected<DeleteData, JsonError>>(
            boost::json::parse(data));
        if (res.has_value()) {
            handler_.Upsert(
                context_, res.value().key,
                ItemDescriptor(data_model::Tombstone(res.value().version)));
            return MessageStatus::kMessageHandled;
        }
        LD_LOG(logger_, LogLevel::kError) << kErrorDeleteInvalid;
        status_manager_.SetError(
            DataSourceStatus::ErrorInfo::ErrorKind::kInvalidData,
            kErrorDeleteInvalid);
        return MessageStatus::kInvalidMessage;
    }
    return MessageStatus::kUnhandledVerb;
}
}  // namespace launchdarkly::client_side::data_sources
