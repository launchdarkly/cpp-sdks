#include "polling_synchronizer.hpp"

#include <launchdarkly/network/http_error_messages.hpp>
#include <launchdarkly/network/http_requester.hpp>
#include <launchdarkly/server_side/config/builders/all_builders.hpp>

#include <boost/asio/post.hpp>
#include <boost/json.hpp>

#include <algorithm>

namespace launchdarkly::server_side::data_systems {

static char const* const kIdentity = "FDv2 polling synchronizer";
static char const* const kFDv2PollPath = "/sdk/poll";

static char const* const kErrorParsingBody =
    "Could not parse FDv2 polling response";
static char const* const kErrorMissingEvents =
    "FDv2 polling response missing 'events' array";
static char const* const kErrorIncompletePayload =
    "FDv2 polling response did not contain a complete payload";

// Minimum polling interval to prevent accidentally hammering the service.
static constexpr std::chrono::seconds kMinPollInterval{30};

using ErrorInfo = data_interfaces::FDv2SourceResult::ErrorInfo;
using ErrorKind = ErrorInfo::ErrorKind;

static ErrorInfo MakeError(ErrorKind kind,
                           ErrorInfo::StatusCodeType status,
                           std::string message) {
    return ErrorInfo{kind, status, std::move(message),
                     std::chrono::system_clock::now()};
}

FDv2PollingSynchronizer::FDv2PollingSynchronizer(
    boost::asio::any_io_executor const& executor,
    Logger const& logger,
    config::built::ServiceEndpoints const& endpoints,
    config::built::HttpProperties const& http_properties,
    std::optional<std::string> filter_key,
    std::chrono::seconds poll_interval)
    : logger_(logger),
      requester_(executor, http_properties.Tls()),
      endpoints_(endpoints),
      http_properties_(http_properties),
      filter_key_(std::move(filter_key)),
      poll_interval_(std::max(poll_interval, kMinPollInterval)),
      timer_(executor) {
    if (poll_interval < kMinPollInterval) {
        LD_LOG(logger_, LogLevel::kWarn)
            << kIdentity << ": polling interval too frequent, defaulting to "
            << kMinPollInterval.count() << " seconds";
    }
}

network::HttpRequest FDv2PollingSynchronizer::MakeRequest(
    data_model::Selector const& selector) const {
    auto url = std::make_optional(endpoints_.PollingBaseUrl());
    url = network::AppendUrl(url, kFDv2PollPath);

    bool has_query = false;
    if (selector.value) {
        url->append("?basis=" + selector.value->state);
        has_query = true;
    }

    if (filter_key_) {
        url->append(has_query ? "&filter=" : "?filter=");
        url->append(*filter_key_);
    }

    config::builders::HttpPropertiesBuilder const builder(http_properties_);
    return {url.value_or(""), network::HttpMethod::kGet, builder.Build(),
            network::HttpRequest::BodyType{}};
}

void FDv2PollingSynchronizer::DoPoll(data_model::Selector selector) {
    last_poll_start_ = std::chrono::steady_clock::now();
    protocol_handler_.Reset();

    auto request = MakeRequest(selector);
    requester_.Request(request, [this](network::HttpResult res) {
        std::lock_guard guard(mutex_);
        result_ = std::move(res);
        cv_.notify_one();
    });
}

data_interfaces::FDv2SourceResult FDv2PollingSynchronizer::Next(
    std::chrono::milliseconds timeout,
    data_model::Selector selector) {
    std::unique_lock lock(mutex_);

    if (closed_) {
        return data_interfaces::FDv2SourceResult{
            data_interfaces::FDv2SourceResult::Shutdown{}};
    }

    result_.reset();

    if (!started_) {
        started_ = true;
        // First call: poll immediately (post to avoid holding the lock).
        boost::asio::post(timer_.get_executor(),
                          [this, selector] { DoPoll(selector); });
    } else {
        // Subsequent calls: schedule next poll after the remaining interval.
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - last_poll_start_);
        auto delay = std::chrono::seconds(
            std::max(poll_interval_ - elapsed, std::chrono::seconds(0)));

        timer_.cancel();
        timer_.expires_after(delay);
        timer_.async_wait(
            [this, selector](boost::system::error_code const& ec) {
                if (ec == boost::asio::error::operation_aborted) {
                    return;
                }
                DoPoll(selector);
            });
    }

    auto deadline = std::chrono::steady_clock::now() + timeout;
    bool timed_out = !cv_.wait_until(
        lock, deadline, [this] { return result_.has_value() || closed_; });

    if (closed_) {
        return data_interfaces::FDv2SourceResult{
            data_interfaces::FDv2SourceResult::Shutdown{}};
    }

    if (timed_out) {
        return data_interfaces::FDv2SourceResult{
            data_interfaces::FDv2SourceResult::Timeout{}};
    }

    return HandlePollResult(*result_);
}

void FDv2PollingSynchronizer::Close() {
    timer_.cancel();
    std::lock_guard lock(mutex_);
    closed_ = true;
    cv_.notify_one();
}

std::string const& FDv2PollingSynchronizer::Identity() const {
    static std::string const identity = kIdentity;
    return identity;
}

data_interfaces::FDv2SourceResult FDv2PollingSynchronizer::HandlePollResult(
    network::HttpResult const& res) {
    if (res.IsError()) {
        auto const& msg = res.ErrorMessage();
        std::string error_msg = msg.has_value() ? *msg : "unknown error";
        LD_LOG(logger_, LogLevel::kWarn)
            << kIdentity << ": " << error_msg;
        return data_interfaces::FDv2SourceResult{
            data_interfaces::FDv2SourceResult::Interrupted{
                MakeError(ErrorKind::kNetworkError, 0, std::move(error_msg)),
                false}};
    }

    if (res.Status() == 304) {
        return data_interfaces::FDv2SourceResult{
            data_interfaces::FDv2SourceResult::ChangeSet{
                data_model::FDv2ChangeSet{
                    data_model::FDv2ChangeSet::Type::kNone, {}, {}},
                false}};
    }

    if (res.Status() == 200) {
        auto const& body = res.Body();
        if (!body) {
            return data_interfaces::FDv2SourceResult{
                data_interfaces::FDv2SourceResult::Interrupted{
                    MakeError(ErrorKind::kInvalidData, 0,
                              "polling response contained no body"),
                    false}};
        }

        boost::system::error_code ec;
        auto parsed = boost::json::parse(*body, ec);
        if (ec) {
            LD_LOG(logger_, LogLevel::kError) << kErrorParsingBody;
            return data_interfaces::FDv2SourceResult{
                data_interfaces::FDv2SourceResult::Interrupted{
                    MakeError(ErrorKind::kInvalidData, 0, kErrorParsingBody),
                    false}};
        }

        auto const* obj = parsed.if_object();
        if (!obj) {
            LD_LOG(logger_, LogLevel::kError) << kErrorParsingBody;
            return data_interfaces::FDv2SourceResult{
                data_interfaces::FDv2SourceResult::Interrupted{
                    MakeError(ErrorKind::kInvalidData, 0, kErrorParsingBody),
                    false}};
        }

        auto const* events_val = obj->if_contains("events");
        if (!events_val) {
            LD_LOG(logger_, LogLevel::kError) << kErrorMissingEvents;
            return data_interfaces::FDv2SourceResult{
                data_interfaces::FDv2SourceResult::Interrupted{
                    MakeError(ErrorKind::kInvalidData, 0, kErrorMissingEvents),
                    false}};
        }

        auto const* events_arr = events_val->if_array();
        if (!events_arr) {
            LD_LOG(logger_, LogLevel::kError) << kErrorMissingEvents;
            return data_interfaces::FDv2SourceResult{
                data_interfaces::FDv2SourceResult::Interrupted{
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
                return data_interfaces::FDv2SourceResult{
                    data_interfaces::FDv2SourceResult::ChangeSet{
                        std::move(*changeset), false}};
            }
            if (auto* goodbye = std::get_if<Goodbye>(&result)) {
                return data_interfaces::FDv2SourceResult{
                    data_interfaces::FDv2SourceResult::Goodbye{goodbye->reason,
                                                               false}};
            }
            if (auto* error = std::get_if<FDv2Error>(&result)) {
                std::string msg = "Server error: " + error->reason;
                LD_LOG(logger_, LogLevel::kInfo)
                    << kIdentity << ": " << msg;
                return data_interfaces::FDv2SourceResult{
                    data_interfaces::FDv2SourceResult::Interrupted{
                        MakeError(ErrorKind::kUnknown, 0, std::move(msg)),
                        false}};
            }
        }

        LD_LOG(logger_, LogLevel::kError) << kErrorIncompletePayload;
        return data_interfaces::FDv2SourceResult{
            data_interfaces::FDv2SourceResult::Interrupted{
                MakeError(ErrorKind::kInvalidData, 0, kErrorIncompletePayload),
                false}};
    }

    if (network::IsRecoverableStatus(res.Status())) {
        std::string msg = network::ErrorForStatusCode(
            res.Status(), "FDv2 polling request", "will retry");
        LD_LOG(logger_, LogLevel::kWarn) << kIdentity << ": " << msg;
        return data_interfaces::FDv2SourceResult{
            data_interfaces::FDv2SourceResult::Interrupted{
                MakeError(ErrorKind::kErrorResponse, res.Status(),
                          std::move(msg)),
                false}};
    }

    std::string msg = network::ErrorForStatusCode(
        res.Status(), "FDv2 polling request", std::nullopt);
    LD_LOG(logger_, LogLevel::kError) << kIdentity << ": " << msg;
    return data_interfaces::FDv2SourceResult{
        data_interfaces::FDv2SourceResult::TerminalError{
            MakeError(ErrorKind::kErrorResponse, res.Status(), std::move(msg)),
            false}};
}

}  // namespace launchdarkly::server_side::data_systems
