#include "events/detail/request_worker.hpp"
#include "events/detail/parse_date_header.hpp"

namespace launchdarkly::events::detail {

RequestWorker::RequestWorker(boost::asio::any_io_executor io,
                             std::chrono::milliseconds retry_after,
                             ServerTimeCallback server_time_cb,
                             PermanentFailureCallback permanent_failure_cb,
                             Logger& logger)
    : timer_(boost::asio::make_strand(io)),
      retry_delay_(retry_after),
      state_(State::Available),
      state_lock_(),
      requester_(timer_.get_executor()),
      request_(std::nullopt),
      server_time_cb_(std::move(server_time_cb)),
      permanent_failure_cb_(std::move(permanent_failure_cb)),
      logger_(logger) {}

bool RequestWorker::Available() const {
    std::lock_guard<std::mutex> guard{state_lock_};
    return state_ == State::Available;
}

void RequestWorker::AsyncDeliver(network::detail::HttpRequest request) {
    UpdateState(State::FirstChance);
    request_ = request;
    requester_.Request(std::move(request),
                       [this](network::detail::HttpResult result) {
                           OnDeliveryAttempt(std::move(result));
                       });
}

static bool IsRecoverableFailure(network::detail::HttpResult const& result) {
    auto status = http::status(result.Status());

    if (result.IsError() ||
        http::to_status_class(status) != http::status_class::client_error) {
        return true;
    }

    return status == http::status::bad_request ||
           status == http::status::request_timeout ||
           status == http::status::too_many_requests;
}

static bool IsSuccess(network::detail::HttpResult const& result) {
    return !result.IsError() &&
           http::to_status_class(http::status(result.Status())) ==
               http::status_class::successful;
}

void RequestWorker::OnDeliveryAttempt(network::detail::HttpResult result) {
    auto [next_state, action] = NextState(state_, result);

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

    UpdateState(next_state);
}

void RequestWorker::UpdateState(State new_state) {
    std::lock_guard<std::mutex> guard{state_lock_};
    state_ = new_state;
}

std::pair<State, Action> NextState(State state,
                                   network::detail::HttpResult const& result) {
    Action action = Action::None;
    State next_state = State::Unknown;
    switch (state) {
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
            assert("impossible state reached" && 0);
    }
    return {next_state, action};
}

std::ostream& operator<<(std::ostream& out, State const& s) {
    switch (s) {
        case State::Unknown:
            out << "State::Unknown";
            break;
        case State::Available:
            out << "State::Available";
            break;
        case State::FirstChance:
            out << "State::FirstChance";
            break;
        case State::SecondChance:
            out << "State::SecondChance";
            break;
        case State::PermanentlyFailed:
            out << "State::PermanentlyFailed";
            break;
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, Action const& s) {
    switch (s) {
        case Action::None:
            out << "Action::None";
            break;
        case Action::Reset:
            out << "Action::Reset";
            break;
        case Action::ParseDateAndReset:
            out << "Action::ParseDateAndReset";
            break;
        case Action::Retry:
            out << "Action::Retry";
            break;
        case Action::NotifyPermanentFailure:
            out << "Action::NotifyPermanentFailure";
            break;
    }
    return out;
}

}  // namespace launchdarkly::events::detail
