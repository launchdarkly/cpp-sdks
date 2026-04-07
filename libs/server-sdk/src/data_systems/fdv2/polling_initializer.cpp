#include "polling_initializer.hpp"

#include <launchdarkly/network/http_error_messages.hpp>
#include <launchdarkly/network/http_requester.hpp>
#include <launchdarkly/server_side/config/builders/all_builders.hpp>

#include <boost/json.hpp>

#include <memory>

namespace launchdarkly::server_side::data_systems {

static char const* const kIdentity = "FDv2 polling initializer";
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

static network::HttpRequest MakeRequest(
    Logger const& logger,
    config::built::ServiceEndpoints const& endpoints,
    config::built::HttpProperties const& http_properties,
    data_model::Selector const& selector,
    std::optional<std::string> const& filter_key) {
    auto url = std::make_optional(endpoints.PollingBaseUrl());
    url = network::AppendUrl(url, kFDv2PollPath);

    bool has_query = false;
    if (selector.value) {
        url->append("?basis=" + selector.value->state);
        has_query = true;
    }

    if (filter_key) {
        url->append(has_query ? "&filter=" : "?filter=");
        url->append(*filter_key);
    }

    config::builders::HttpPropertiesBuilder const builder(http_properties);
    return {url.value_or(""), network::HttpMethod::kGet, builder.Build(),
            network::HttpRequest::BodyType{}};
}

FDv2PollingInitializer::FDv2PollingInitializer(
    boost::asio::any_io_executor const& executor,
    Logger const& logger,
    config::built::ServiceEndpoints const& endpoints,
    config::built::HttpProperties const& http_properties,
    data_model::Selector selector,
    std::optional<std::string> filter_key)
    : logger_(logger),
      request_(MakeRequest(logger,
                           endpoints,
                           http_properties,
                           selector,
                           filter_key)),
      requester_(executor, http_properties.Tls()) {}

FDv2SourceResult FDv2PollingInitializer::Run() {
    if (!request_.Valid()) {
        LD_LOG(logger_, LogLevel::kError)
            << kIdentity << ": invalid polling endpoint URL";
        return FDv2SourceResult{FDv2SourceResult::TerminalError{
            MakeError(ErrorKind::kUnknown, 0, "invalid polling endpoint URL"),
            false}};
    }

    auto shared_result = std::make_shared<std::optional<network::HttpResult>>();

    requester_.Request(request_,
                       [this, shared_result](network::HttpResult res) {
                           std::lock_guard guard(mutex_);
                           *shared_result = std::move(res);
                           cv_.notify_one();
                       });

    std::unique_lock lock(mutex_);
    cv_.wait(lock, [&] { return shared_result->has_value() || closed_; });

    if (closed_) {
        return FDv2SourceResult{FDv2SourceResult::Shutdown{}};
    }

    auto http_result = std::move(**shared_result);
    lock.unlock();

    return HandlePollResult(http_result);
}

void FDv2PollingInitializer::Close() {
    std::lock_guard lock(mutex_);
    closed_ = true;
    cv_.notify_one();
}

std::string const& FDv2PollingInitializer::Identity() const {
    static std::string const identity = kIdentity;
    return identity;
}

FDv2SourceResult FDv2PollingInitializer::HandlePollResult(
    network::HttpResult const& res) {
    if (res.IsError()) {
        auto const& msg = res.ErrorMessage();
        std::string error_msg = msg.has_value() ? *msg : "unknown error";
        LD_LOG(logger_, LogLevel::kWarn) << kIdentity << ": " << error_msg;
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
            LD_LOG(logger_, LogLevel::kError) << kErrorParsingBody;
            return FDv2SourceResult{FDv2SourceResult::Interrupted{
                MakeError(ErrorKind::kInvalidData, 0, kErrorParsingBody),
                false}};
        }

        auto const* obj = parsed.if_object();
        if (!obj) {
            LD_LOG(logger_, LogLevel::kError) << kErrorParsingBody;
            return FDv2SourceResult{FDv2SourceResult::Interrupted{
                MakeError(ErrorKind::kInvalidData, 0, kErrorParsingBody),
                false}};
        }

        auto const* events_val = obj->if_contains("events");
        if (!events_val) {
            LD_LOG(logger_, LogLevel::kError) << kErrorMissingEvents;
            return FDv2SourceResult{FDv2SourceResult::Interrupted{
                MakeError(ErrorKind::kInvalidData, 0, kErrorMissingEvents),
                false}};
        }

        auto const* events_arr = events_val->if_array();
        if (!events_arr) {
            LD_LOG(logger_, LogLevel::kError) << kErrorMissingEvents;
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

            auto result = protocol_handler_.HandleEvent(
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
                    auto const& id = error->server_error->id;
                    std::string msg =
                        "An issue was encountered receiving updates for "
                        "payload '" +
                        id.value_or("") + "' with reason: '" + error->message +
                        "'. Automatic retry will occur.";
                    LD_LOG(logger_, LogLevel::kInfo)
                        << kIdentity << ": " << msg;
                    return FDv2SourceResult{FDv2SourceResult::Interrupted{
                        MakeError(ErrorKind::kErrorResponse, 0, std::move(msg)),
                        false}};
                }
                LD_LOG(logger_, LogLevel::kError)
                    << kIdentity << ": " << error->message;
                return FDv2SourceResult{FDv2SourceResult::Interrupted{
                    MakeError(ErrorKind::kInvalidData, 0, error->message),
                    false}};
            }
        }

        LD_LOG(logger_, LogLevel::kError) << kErrorIncompletePayload;
        return FDv2SourceResult{FDv2SourceResult::Interrupted{
            MakeError(ErrorKind::kInvalidData, 0, kErrorIncompletePayload),
            false}};
    }

    if (network::IsRecoverableStatus(res.Status())) {
        std::string msg = network::ErrorForStatusCode(
            res.Status(), "FDv2 polling request", "will retry");
        LD_LOG(logger_, LogLevel::kWarn) << kIdentity << ": " << msg;
        return FDv2SourceResult{FDv2SourceResult::Interrupted{
            MakeError(ErrorKind::kErrorResponse, res.Status(), std::move(msg)),
            false}};
    }

    std::string msg = network::ErrorForStatusCode(
        res.Status(), "FDv2 polling request", std::nullopt);
    LD_LOG(logger_, LogLevel::kError) << kIdentity << ": " << msg;
    return FDv2SourceResult{FDv2SourceResult::TerminalError{
        MakeError(ErrorKind::kErrorResponse, res.Status(), std::move(msg)),
        false}};
}

}  // namespace launchdarkly::server_side::data_systems
