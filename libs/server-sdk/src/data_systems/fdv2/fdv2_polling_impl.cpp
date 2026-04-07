#include "fdv2_polling_impl.hpp"

#include <launchdarkly/network/http_error_messages.hpp>
#include <launchdarkly/server_side/config/builders/all_builders.hpp>

#include <boost/json.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/url.hpp>

namespace launchdarkly::server_side::data_systems {

static char const* const kFDv2PollPath = "/sdk/poll";

static char const* const kErrorParsingBody =
    "Could not parse FDv2 polling response";
static char const* const kErrorMissingEvents =
    "FDv2 polling response missing 'events' array";
static char const* const kErrorIncompletePayload =
    "FDv2 polling response did not contain a complete payload";

using data_interfaces::FDv2SourceResult;
using ErrorInfo = FDv2SourceResult::ErrorInfo;
using ErrorKind = ErrorInfo::ErrorKind;

static ErrorInfo MakeError(ErrorKind kind,
                           ErrorInfo::StatusCodeType status,
                           std::string message) {
    return ErrorInfo{kind, status, std::move(message),
                     std::chrono::system_clock::now()};
}

network::HttpRequest MakeFDv2PollRequest(
    config::built::ServiceEndpoints const& endpoints,
    config::built::HttpProperties const& http_properties,
    data_model::Selector const& selector,
    std::optional<std::string> const& filter_key) {
    config::builders::HttpPropertiesBuilder const builder(http_properties);

    auto parsed = boost::urls::parse_uri(endpoints.PollingBaseUrl());
    if (!parsed) {
        return {"", network::HttpMethod::kGet, builder.Build(),
                network::HttpRequest::BodyType{}};
    }

    boost::urls::url u = parsed.value();
    u.set_path(u.path() + kFDv2PollPath);
    if (selector.value) {
        u.params().append({"basis", selector.value->state});
    }
    if (filter_key) {
        u.params().append({"filter", *filter_key});
    }

    return {std::string(u.buffer()), network::HttpMethod::kGet, builder.Build(),
            network::HttpRequest::BodyType{}};
}

data_interfaces::FDv2SourceResult HandleFDv2PollResponse(
    network::HttpResult const& res,
    FDv2ProtocolHandler& protocol_handler,
    Logger const& logger,
    std::string_view identity) {
    if (res.IsError()) {
        auto const& msg = res.ErrorMessage();
        std::string error_msg = msg.has_value() ? *msg : "unknown error";
        LD_LOG(logger, LogLevel::kWarn) << identity << ": " << error_msg;
        return FDv2SourceResult{FDv2SourceResult::Interrupted{
            MakeError(ErrorKind::kNetworkError, 0, std::move(error_msg)),
            false}};
    }

    if (res.Status() == 304) {
        return FDv2SourceResult{FDv2SourceResult::ChangeSet{
            data_model::FDv2ChangeSet{
                data_model::FDv2ChangeSet::Type::kNone, {}, {}},
            false}};
    }

    if (res.Status() == 200) {
        auto const& body = res.Body();
        if (!body) {
            return FDv2SourceResult{FDv2SourceResult::Interrupted{
                MakeError(ErrorKind::kInvalidData, 0,
                          "polling response contained no body"),
                false}};
        }

        boost::system::error_code ec;
        auto parsed = boost::json::parse(*body, ec);
        if (ec) {
            LD_LOG(logger, LogLevel::kError) << kErrorParsingBody;
            return FDv2SourceResult{FDv2SourceResult::Interrupted{
                MakeError(ErrorKind::kInvalidData, 0, kErrorParsingBody),
                false}};
        }

        auto const* obj = parsed.if_object();
        if (!obj) {
            LD_LOG(logger, LogLevel::kError) << kErrorParsingBody;
            return FDv2SourceResult{FDv2SourceResult::Interrupted{
                MakeError(ErrorKind::kInvalidData, 0, kErrorParsingBody),
                false}};
        }

        auto const* events_val = obj->if_contains("events");
        if (!events_val) {
            LD_LOG(logger, LogLevel::kError) << kErrorMissingEvents;
            return FDv2SourceResult{FDv2SourceResult::Interrupted{
                MakeError(ErrorKind::kInvalidData, 0, kErrorMissingEvents),
                false}};
        }

        auto const* events_arr = events_val->if_array();
        if (!events_arr) {
            LD_LOG(logger, LogLevel::kError) << kErrorMissingEvents;
            return FDv2SourceResult{FDv2SourceResult::Interrupted{
                MakeError(ErrorKind::kInvalidData, 0, kErrorMissingEvents),
                false}};
        }

        for (auto const& event_val : *events_arr) {
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

            auto result = protocol_handler.HandleEvent(
                std::string_view{event_type_str->data(),
                                 event_type_str->size()},
                *event_data_val);

            if (auto* changeset =
                    std::get_if<data_model::FDv2ChangeSet>(&result)) {
                return FDv2SourceResult{
                    FDv2SourceResult::ChangeSet{std::move(*changeset), false}};
            }
            if (auto* goodbye = std::get_if<Goodbye>(&result)) {
                return FDv2SourceResult{
                    FDv2SourceResult::Goodbye{goodbye->reason, false}};
            }
            if (auto* error =
                    std::get_if<FDv2ProtocolHandler::Error>(&result)) {
                if (error->kind ==
                    FDv2ProtocolHandler::Error::Kind::kServerError) {
                    auto const& id = error->server_error.value().id;
                    std::string msg =
                        "An issue was encountered receiving updates for "
                        "payload '" +
                        id.value_or("") + "' with reason: '" + error->message +
                        "'. Automatic retry will occur.";
                    LD_LOG(logger, LogLevel::kInfo) << identity << ": " << msg;
                    return FDv2SourceResult{FDv2SourceResult::Interrupted{
                        MakeError(ErrorKind::kErrorResponse, 0, std::move(msg)),
                        false}};
                }
                LD_LOG(logger, LogLevel::kError)
                    << identity << ": " << error->message;
                return FDv2SourceResult{FDv2SourceResult::Interrupted{
                    MakeError(ErrorKind::kInvalidData, 0, error->message),
                    false}};
            }
        }

        LD_LOG(logger, LogLevel::kError) << kErrorIncompletePayload;
        return FDv2SourceResult{FDv2SourceResult::Interrupted{
            MakeError(ErrorKind::kInvalidData, 0, kErrorIncompletePayload),
            false}};
    }

    if (network::IsRecoverableStatus(res.Status())) {
        std::string msg = network::ErrorForStatusCode(
            res.Status(), "FDv2 polling request", "will retry");
        LD_LOG(logger, LogLevel::kWarn) << identity << ": " << msg;
        return FDv2SourceResult{FDv2SourceResult::Interrupted{
            MakeError(ErrorKind::kErrorResponse, res.Status(), std::move(msg)),
            false}};
    }

    std::string msg = network::ErrorForStatusCode(
        res.Status(), "FDv2 polling request", std::nullopt);
    LD_LOG(logger, LogLevel::kError) << identity << ": " << msg;
    return FDv2SourceResult{FDv2SourceResult::TerminalError{
        MakeError(ErrorKind::kErrorResponse, res.Status(), std::move(msg)),
        false}};
}

}  // namespace launchdarkly::server_side::data_systems
