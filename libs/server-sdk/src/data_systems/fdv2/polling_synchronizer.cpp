#include "polling_synchronizer.hpp"

#include <launchdarkly/network/http_error_messages.hpp>
#include <launchdarkly/network/http_requester.hpp>
#include <launchdarkly/server_side/config/builders/all_builders.hpp>

#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/asio/post.hpp>
#include <boost/json.hpp>

#include <algorithm>
#include <future>
#include <memory>

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

using data_interfaces::FDv2SourceResult;
using ErrorInfo = FDv2SourceResult::ErrorInfo;
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

FDv2SourceResult FDv2PollingSynchronizer::Next(
    std::chrono::milliseconds timeout,
    data_model::Selector selector) {
    if (closed_) {
        return FDv2SourceResult{FDv2SourceResult::Shutdown{}};
    }

    auto promise = std::make_shared<std::promise<FDv2SourceResult>>();
    auto future = promise->get_future();

    // Post the actual work to the ASIO thread so that timer_ and
    // cancel_signal_ are only ever accessed from the executor. Calling their
    // methods directly here (on the orchestrator thread) would be a data race,
    // since ASIO I/O objects and cancellation_signal are not thread-safe.
    // future.get() below blocks the orchestrator thread until DoNext/DoPoll
    // sets the promise from the ASIO thread.
    boost::asio::post(
        timer_.get_executor(),
        [this, timeout, selector = std::move(selector), promise]() mutable {
            DoNext(timeout, std::move(selector), std::move(promise));
        });

    return future.get();
}

void FDv2PollingSynchronizer::DoNext(
    std::chrono::milliseconds timeout,
    data_model::Selector selector,
    std::shared_ptr<std::promise<FDv2SourceResult>> promise) {
    if (closed_) {
        promise->set_value(FDv2SourceResult{FDv2SourceResult::Shutdown{}});
        return;
    }

    auto deadline = std::chrono::steady_clock::now() + timeout;

    if (started_) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - last_poll_start_);
        auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(
                         poll_interval_) -
                     elapsed;

        if (delay.count() > 0) {
            auto remaining =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    deadline - std::chrono::steady_clock::now());

            // timeout_timer must outlive this function: the io_context's
            // internal timer heap holds a pointer to the timer's
            // implementation until the async_wait completes, and the
            // destructor cancels any pending async_wait (posting
            // operation_aborted), which would make the parallel_group
            // immediately complete as if the timeout had fired. Capturing
            // the shared_ptr in the callback lambda keeps the timer alive
            // until the group completes.
            auto timeout_timer = std::make_shared<boost::asio::steady_timer>(
                timer_.get_executor());
            timer_.expires_after(delay);
            timeout_timer->expires_after(remaining);

            boost::asio::experimental::make_parallel_group(
                timer_.async_wait(boost::asio::deferred),
                timeout_timer->async_wait(boost::asio::deferred))
                .async_wait(boost::asio::experimental::wait_for_one(),
                            boost::asio::bind_cancellation_slot(
                                cancel_signal_.slot(),
                                [this, deadline, selector = std::move(selector),
                                 promise, timeout_timer](
                                    std::array<std::size_t, 2> order,
                                    boost::system::error_code,
                                    boost::system::error_code) mutable {
                                    if (closed_) {
                                        promise->set_value(FDv2SourceResult{
                                            FDv2SourceResult::Shutdown{}});
                                        return;
                                    }
                                    if (order[0] == 1) {
                                        promise->set_value(FDv2SourceResult{
                                            FDv2SourceResult::Timeout{}});
                                        return;
                                    }
                                    DoPoll(deadline, std::move(selector),
                                           std::move(promise));
                                }));
            return;
        }
    }

    DoPoll(deadline, std::move(selector), std::move(promise));
}

void FDv2PollingSynchronizer::DoPoll(
    std::chrono::time_point<std::chrono::steady_clock> deadline,
    data_model::Selector selector,
    std::shared_ptr<std::promise<FDv2SourceResult>> promise) {
    if (closed_) {
        promise->set_value(FDv2SourceResult{FDv2SourceResult::Shutdown{}});
        return;
    }

    started_ = true;
    last_poll_start_ = std::chrono::steady_clock::now();
    protocol_handler_.Reset();

    auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
        deadline - std::chrono::steady_clock::now());
    timer_.expires_after(remaining);

    boost::asio::experimental::make_parallel_group(
        requester_.Request(MakeRequest(selector), boost::asio::deferred),
        timer_.async_wait(boost::asio::deferred))
        .async_wait(
            boost::asio::experimental::wait_for_one(),
            boost::asio::bind_cancellation_slot(
                cancel_signal_.slot(),
                [this, promise](std::array<std::size_t, 2> order,
                                network::HttpResult poll_result,
                                boost::system::error_code) mutable {
                    if (order[0] == 0) {
                        promise->set_value(HandlePollResult(poll_result));
                    } else if (closed_) {
                        promise->set_value(
                            FDv2SourceResult{FDv2SourceResult::Shutdown{}});
                    } else {
                        promise->set_value(
                            FDv2SourceResult{FDv2SourceResult::Timeout{}});
                    }
                }));
}

void FDv2PollingSynchronizer::Close() {
    closed_ = true;
    // cancel_signal_ is not thread-safe, so emit() must run on the ASIO
    // thread. post() schedules it there rather than calling it directly,
    // which would race with DoNext/DoPoll accessing the signal concurrently.
    boost::asio::post(timer_.get_executor(), [this] {
        cancel_signal_.emit(boost::asio::cancellation_type::all);
    });
}

std::string const& FDv2PollingSynchronizer::Identity() const {
    static std::string const identity = kIdentity;
    return identity;
}

FDv2SourceResult FDv2PollingSynchronizer::HandlePollResult(
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
