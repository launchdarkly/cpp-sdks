#include "launchdarkly/client_side/data_sources/detail/streaming_data_handler.hpp"
#include "launchdarkly/client_side/data_sources/detail/base_64.hpp"
#include "launchdarkly/client_side/data_sources/detail/streaming_data_source.hpp"
#include "serialization/json_evaluation_result.hpp"
#include "serialization/value_mapping.hpp"

#include <boost/json.hpp>
#include <utility>

#include "tl/expected.hpp"

namespace launchdarkly::client_side {
// This tag_invoke needs to be in the same namespace as the
// ItemDescriptor.

static tl::expected<
    std::unordered_map<std::string, launchdarkly::client_side::ItemDescriptor>,
    JsonError>
tag_invoke(boost::json::value_to_tag<tl::expected<
               std::unordered_map<std::string,
                                  launchdarkly::client_side::ItemDescriptor>,
               JsonError>> const& unused,
           boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    if (!json_value.is_object()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    auto const& obj = json_value.as_object();
    std::unordered_map<std::string, launchdarkly::client_side::ItemDescriptor>
        descriptors;
    for (auto const& pair : obj) {
        auto eval_result =
            boost::json::value_to<tl::expected<EvaluationResult, JsonError>>(
                pair.value());
        if (!eval_result.has_value()) {
            return tl::unexpected(JsonError::kSchemaFailure);
        }
        descriptors.emplace(pair.key(),
                            launchdarkly::client_side::ItemDescriptor(
                                std::move(eval_result.value())));
    }
    return descriptors;
}
}  // namespace launchdarkly::client_side

namespace launchdarkly::client_side::data_sources::detail {

static tl::expected<StreamingDataHandler::PatchData, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<StreamingDataHandler::PatchData, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    if (json_value.is_object()) {
        auto const& obj = json_value.as_object();
        auto const* key_iter = obj.find("key");
        auto key = ValueAsOpt<std::string>(key_iter, obj.end());
        auto result =
            boost::json::value_to<tl::expected<EvaluationResult, JsonError>>(
                json_value);

        if (result.has_value() && key.has_value()) {
            return StreamingDataHandler::PatchData{key.value(), result.value()};
        }
    }
    return tl::unexpected(JsonError::kSchemaFailure);
}

static tl::expected<StreamingDataHandler::DeleteData, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<StreamingDataHandler::DeleteData,
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
            return StreamingDataHandler::DeleteData{key.value(),
                                                    version.value()};
        }
    }
    return tl::unexpected(JsonError::kSchemaFailure);
}

StreamingDataHandler::StreamingDataHandler(
    std::shared_ptr<IDataSourceUpdateSink> handler,
    Logger const& logger)
    : handler_(std::move(std::move(std::move(handler)))), logger_(logger) {}

StreamingDataHandler::MessageStatus StreamingDataHandler::handle_message(
    launchdarkly::sse::Event const& event) {
    if (event.type() == "put") {
        boost::json::error_code error_code;
        auto parsed = boost::json::parse(event.data(), error_code);
        if (error_code) {
            LD_LOG(logger_, LogLevel::kError) << "Could not parse PUT message";
            return StreamingDataHandler::MessageStatus::kInvalidMessage;
        }
        auto res = boost::json::value_to<tl::expected<
            std::unordered_map<std::string, ItemDescriptor>, JsonError>>(
            parsed);

        if (res.has_value()) {
            handler_->init(res.value());
            return StreamingDataHandler::MessageStatus::kMessageHandled;
        }
        LD_LOG(logger_, LogLevel::kError)
            << "PUT message contained invalid data";
        return StreamingDataHandler::MessageStatus::kInvalidMessage;
    }
    if (event.type() == "patch") {
        boost::json::error_code error_code;
        auto parsed = boost::json::parse(event.data(), error_code);
        if (error_code) {
            LD_LOG(logger_, LogLevel::kError)
                << "Could not parse PATCH message";
            return StreamingDataHandler::MessageStatus::kInvalidMessage;
        }
        auto res = boost::json::value_to<
            tl::expected<StreamingDataHandler::PatchData, JsonError>>(parsed);
        if (res.has_value()) {
            handler_->upsert(
                res.value().key,
                launchdarkly::client_side::ItemDescriptor(res.value().flag));
            return StreamingDataHandler::MessageStatus::kMessageHandled;
        }
        LD_LOG(logger_, LogLevel::kError)
            << "PATCH message contained invalid data";
        return StreamingDataHandler::MessageStatus::kInvalidMessage;
    }
    if (event.type() == "delete") {
        boost::json::error_code error_code;
        auto parsed = boost::json::parse(event.data(), error_code);
        if (error_code) {
            LD_LOG(logger_, LogLevel::kError)
                << "Could not parse DELETE message";
            return StreamingDataHandler::MessageStatus::kInvalidMessage;
        }
        auto res = boost::json::value_to<
            tl::expected<StreamingDataHandler::DeleteData, JsonError>>(
            boost::json::parse(event.data()));
        if (res.has_value()) {
            handler_->upsert(res.value().key,
                             ItemDescriptor(res.value().version));
            return StreamingDataHandler::MessageStatus::kMessageHandled;
        }
        LD_LOG(logger_, LogLevel::kError)
            << "DELETE message contained invalid data";
        return StreamingDataHandler::MessageStatus::kInvalidMessage;
    }
    return StreamingDataHandler::MessageStatus::kUnhandledVerb;
}
}  // namespace launchdarkly::client_side::data_sources::detail
