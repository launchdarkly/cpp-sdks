#include "events/detail/request_worker.hpp"
#include "events/detail/parse_date_header.hpp"

namespace launchdarkly::events::detail {
RequestWorker::RequestWorker(boost::asio::any_io_executor io,
                             std::chrono::milliseconds retry_after,
                             Logger& logger,
                             ServerTimeCallback server_time_cb,
                             PermanentFailureCallback permanent_failure_cb)
    : retry_delay_(retry_after),
      state_(State::Available),
      timer_(io),
      requester_(io),
      request_(std::nullopt),
      logger_(logger),
      server_time_cb_(std::move(server_time_cb)),
      permanent_failure_cb_(std::move(permanent_failure_cb)) {}

bool RequestWorker::Available() const {
    return state_ == State::Available;
}

void RequestWorker::AsyncDeliver(network::detail::HttpRequest request) {
    state_ = State::FirstChance;
    request_ = request;
    requester_.Request(std::move(request),
                       [this](network::detail::HttpResult result) {
                           OnDeliveryAttempt(std::move(result));
                       });
}

static bool IsRecoverableFailure(network::detail::HttpResult const& result) {
    if (result.IsError()) {
        // todo(cwaldren): determine how/why IsError is set
        return false;
    }
    auto status = http::status(result.Status());
    return status == http::status::bad_request ||
           status == http::status::request_timeout ||
           status == http::status::too_many_requests;
}

static bool IsSuccess(network::detail::HttpResult const& result) {
    return !result.IsError() && http::status_class(result.Status()) ==
                                    http::status_class::successful;
}

void RequestWorker::OnDeliveryAttempt(network::detail::HttpResult result) {
    Action action = Action::None;
    State next_state = State::Unknown;
    switch (state_) {
        case State::FirstChance:
            if (IsSuccess(result)) {
                next_state = State::Available;
                action = Action::ParseDateAndReset;
            } else if (IsRecoverableFailure(result)) {
                next_state = State::SecondChance;
                action = Action::Retry;
            } else {
                next_state = State::PermanentlyFailed;
                action = Action::NotifyPermanentFailure;
            }
            break;
        case State::SecondChance:
            if (IsSuccess(result)) {
                next_state = State::Available;
                action = Action::ParseDateAndReset;
            } else if (IsRecoverableFailure(result)) {
                // no more delivery attempts; payload is dropped.
                next_state = State::Available;
                action = Action::Reset;
            } else {
                next_state = State::PermanentlyFailed;
                action = Action::NotifyPermanentFailure;
            }
            break;
        default:
            assert(0);
    }

    LD_LOG(logger_, LogLevel::kDebug)
        << "request_worker: state [" << int(state_) << "] --> state ["
        << int(next_state) << "], action (" << int(action) << ")";

    switch (action) {
        case Action::None:
            break;
        case Action::Reset:
            request_.reset();
            break;
        case Action::NotifyPermanentFailure:
            request_.reset();
            permanent_failure_cb_();
            break;
        case Action::ParseDateAndReset: {
            request_.reset();
            LD_LOG(logger_, LogLevel::kDebug)
                << "successfully delivered events";
            auto headers = result.Headers();
            if (auto date_header = headers.find("Date");
                date_header != headers.end()) {
                if (auto datetime = ParseDateHeader<std::chrono::system_clock>(
                        date_header->second)) {
                    server_time_cb_(*datetime);
                }
            }
        } break;
        case Action::Retry:
            timer_.expires_from_now(retry_delay_);
            timer_.async_wait([this](boost::system::error_code ec) {
                if (ec) {
                    return;
                }
                requester_.Request(*request_,
                                   [this](network::detail::HttpResult result) {
                                       OnDeliveryAttempt(std::move(result));
                                   });
            });
            break;
    }

    state_ = next_state;
}

}  // namespace launchdarkly::events::detail
