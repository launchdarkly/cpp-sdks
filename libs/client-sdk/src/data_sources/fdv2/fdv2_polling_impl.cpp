#include "fdv2_polling_impl.hpp"
#include "fdv2_changeset_translation.hpp"
#include "fdv2_response_headers.hpp"

#include <launchdarkly/config/shared/builders/http_properties_builder.hpp>
#include <launchdarkly/config/shared/sdks.hpp>
#include <launchdarkly/encoding/base_64.hpp>
#include <launchdarkly/network/http_error_messages.hpp>

#include <boost/json.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/url.hpp>

#include <utility>

namespace launchdarkly::client_side::data_sources {

static char const* const kErrorParsingBody =
    "Could not parse FDv2 polling response";
static char const* const kErrorMissingEvents =
    "FDv2 polling response missing 'events' array";
static char const* const kErrorIncompletePayload =
    "FDv2 polling response did not contain a complete payload";
static char const* const kErrorTranslation =
    "FDv2 polling response could not be translated";

using ErrorInfo = FDv2SourceResult::ErrorInfo;
using ErrorKind = ErrorInfo::ErrorKind;

static ErrorInfo MakeError(ErrorKind kind,
                           ErrorInfo::StatusCodeType status,
                           std::string message) {
    return ErrorInfo{kind, status, std::move(message),
                     std::chrono::system_clock::now()};
}

network::HttpRequest MakeFDv2PollRequest(FDv2RequestConfig const& config,
                                         data_model::Selector const& selector) {
    config::shared::builders::HttpPropertiesBuilder<config::shared::ClientSDK>
        builder(config.http_properties);

    bool const post = config.transport == FDv2ContextTransport::kPostBody;
    if (post) {
        builder.Header("content-type", "application/json");
    }

    auto parsed = boost::urls::parse_uri(config.base_url);
    if (!parsed) {
        return {"", network::HttpMethod::kGet, builder.Build(),
                network::HttpRequest::BodyType{}};
    }

    boost::urls::url url = parsed.value();
    // A trailing '/' on the base URL appears as an empty final segment.
    // Remove it so the pushed segments do not produce a double slash.
    auto segments = url.segments();
    if (!segments.empty() && segments.back().empty()) {
        segments.pop_back();
    }
    segments.push_back("sdk");
    segments.push_back("poll");
    segments.push_back("eval");
    if (!post) {
        segments.push_back(
            encoding::Base64UrlEncode(config.serialized_context));
    }

    if (selector.value) {
        url.params().append({"basis", selector.value->state});
    }
    if (config.with_reasons) {
        url.params().append({"withReasons", "true"});
    }

    return {std::string(url.buffer()),
            post ? network::HttpMethod::kPost : network::HttpMethod::kGet,
            builder.Build(),
            post ? network::HttpRequest::BodyType{config.serialized_context}
                 : network::HttpRequest::BodyType{}};
}

static FDv2SourceResult ParseFDv2PollEvents(
    boost::json::array const& events,
    FDv2ProtocolHandler* protocol_handler,
    Logger const& logger) {
    for (auto const& event_val : events) {
        auto const* event_obj = event_val.if_object();
        if (!event_obj) {
            continue;
        }

        auto const* event_type_val = event_obj->if_contains("event");
        auto const* event_data_val = event_obj->if_contains("data");
        if (!event_type_val || !event_data_val) {
            continue;
        }

        auto const* event_type_str = event_type_val->if_string();
        if (!event_type_str) {
            continue;
        }

        auto result = protocol_handler->HandleEvent(
            std::string_view{event_type_str->data(), event_type_str->size()},
            *event_data_val);

        if (auto* change_set =
                std::get_if<data_model::FDv2ChangeSet>(&result)) {
            auto typed = TranslateChangeSet(*change_set, logger);
            if (!typed) {
                return FDv2SourceResult{FDv2SourceResult::Interrupted{
                    MakeError(ErrorKind::kInvalidData, 0, kErrorTranslation)}};
            }
            return FDv2SourceResult{
                FDv2SourceResult::ChangeSet{std::move(*typed)}};
        }
        if (auto* goodbye = std::get_if<Goodbye>(&result)) {
            FDv2SourceResult goodbye_result{
                FDv2SourceResult::Goodbye{goodbye->reason}};
            if (goodbye->protocol_fallback_ttl) {
                goodbye_result.fdv1_fallback =
                    FDv1FallbackDirective::FromServiceTtl(
                        std::chrono::seconds(*goodbye->protocol_fallback_ttl));
            }
            return goodbye_result;
        }
        if (auto* error = std::get_if<FDv2ProtocolHandler::Error>(&result)) {
            if (error->kind == FDv2ProtocolHandler::Error::Kind::kServerError) {
                auto const& id = error->server_error.value().id;
                std::string msg =
                    "An issue was encountered receiving updates for "
                    "payload '" +
                    id.value_or("") + "' with reason: '" + error->message +
                    "'. Automatic retry will occur.";
                return FDv2SourceResult{FDv2SourceResult::Interrupted{
                    MakeError(ErrorKind::kErrorResponse, 0, std::move(msg))}};
            }
            return FDv2SourceResult{FDv2SourceResult::Interrupted{
                MakeError(ErrorKind::kInvalidData, 0, error->message)}};
        }
    }

    return FDv2SourceResult{FDv2SourceResult::Interrupted{
        MakeError(ErrorKind::kInvalidData, 0, kErrorIncompletePayload)}};
}

static FDv2SourceResult ParseFDv2PollResponse(
    std::string const& body,
    FDv2ProtocolHandler* protocol_handler,
    Logger const& logger) {
    boost::system::error_code ec;
    auto parsed = boost::json::parse(body, ec);
    if (ec) {
        return FDv2SourceResult{FDv2SourceResult::Interrupted{
            MakeError(ErrorKind::kInvalidData, 0, kErrorParsingBody)}};
    }

    auto const* obj = parsed.if_object();
    if (!obj) {
        return FDv2SourceResult{FDv2SourceResult::Interrupted{
            MakeError(ErrorKind::kInvalidData, 0, kErrorParsingBody)}};
    }

    auto const* events_val = obj->if_contains("events");
    if (!events_val) {
        return FDv2SourceResult{FDv2SourceResult::Interrupted{
            MakeError(ErrorKind::kInvalidData, 0, kErrorMissingEvents)}};
    }

    auto const* events_arr = events_val->if_array();
    if (!events_arr) {
        return FDv2SourceResult{FDv2SourceResult::Interrupted{
            MakeError(ErrorKind::kInvalidData, 0, kErrorMissingEvents)}};
    }

    return ParseFDv2PollEvents(*events_arr, protocol_handler, logger);
}

FDv2SourceResult HandleFDv2PollResponse(network::HttpResult const& res,
                                        FDv2ProtocolHandler* protocol_handler,
                                        Logger const& logger,
                                        std::string_view identity) {
    if (res.IsError()) {
        auto const& msg = res.ErrorMessage();
        std::string error_msg = msg.has_value() ? *msg : "unknown error";
        LD_LOG(logger, LogLevel::kWarn) << identity << ": " << error_msg;
        return FDv2SourceResult{FDv2SourceResult::Interrupted{
            MakeError(ErrorKind::kNetworkError, 0, std::move(error_msg))}};
    }

    auto headers = ReadFDv2ResponseHeaders(res.Headers());

    auto finish = [&headers](FDv2SourceResult result) {
        // A directive parsed from the response body, such as one on a goodbye
        // message, takes precedence over the response header.
        if (!result.fdv1_fallback) {
            result.fdv1_fallback = std::move(headers.fdv1_fallback);
        }
        result.environment_id = std::move(headers.environment_id);
        return result;
    };

    // The SDK's data is confirmed current, which is what a "none" intent
    // means.
    if (res.Status() == 304) {
        return finish(FDv2SourceResult{FDv2SourceResult::ChangeSet{
            FlagChangeSet{data_model::ChangeSetType::kNone,
                          {},
                          data_model::Selector{}}}});
    }

    if (res.Status() == 200) {
        auto const& body = res.Body();
        if (!body) {
            return finish(FDv2SourceResult{FDv2SourceResult::Interrupted{
                MakeError(ErrorKind::kInvalidData, 0,
                          "FDv2 polling response contained no body")}});
        }

        auto result = ParseFDv2PollResponse(*body, protocol_handler, logger);
        if (auto* interrupted =
                std::get_if<FDv2SourceResult::Interrupted>(&result.value)) {
            if (interrupted->error.Kind() == ErrorKind::kErrorResponse) {
                LD_LOG(logger, LogLevel::kInfo)
                    << identity << ": " << interrupted->error.Message();
            } else {
                LD_LOG(logger, LogLevel::kError)
                    << identity << ": " << interrupted->error.Message();
            }
        }
        return finish(std::move(result));
    }

    if (network::IsRecoverableStatus(res.Status())) {
        std::string msg = network::ErrorForStatusCode(
            res.Status(), "FDv2 polling request", "will retry");
        LD_LOG(logger, LogLevel::kWarn) << identity << ": " << msg;
        return finish(FDv2SourceResult{FDv2SourceResult::Interrupted{MakeError(
            ErrorKind::kErrorResponse, res.Status(), std::move(msg))}});
    }

    std::string msg = network::ErrorForStatusCode(
        res.Status(), "FDv2 polling request", std::nullopt);
    LD_LOG(logger, LogLevel::kError) << identity << ": " << msg;
    return finish(FDv2SourceResult{FDv2SourceResult::TerminalError{
        MakeError(ErrorKind::kErrorResponse, res.Status(), std::move(msg))}});
}

}  // namespace launchdarkly::client_side::data_sources
