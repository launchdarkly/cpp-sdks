#include <launchdarkly/events/parse_date_header.hpp>
#include <launchdarkly/events/request_worker.hpp>

namespace launchdarkly::events {

RequestWorker::RequestWorker(boost::asio::any_io_executor io,
                             std::chrono::milliseconds retry_after,
                             std::size_t id,
                             std::optional<std::locale> date_header_locale,
                             Logger& logger)
    : timer_(io),
      retry_delay_(retry_after),
      state_(State::Idle),
      requester_(timer_.get_executor()),
      batch_(std::nullopt),
      tag_("flush-worker[" + std::to_string(id) + "]: "),
      date_header_locale_(std::move(date_header_locale)),
      logger_(logger) {}

bool RequestWorker::Available() const {
    return state_ == State::Idle;
}

// Returns true if the result is considered transient - meaning it should
// not stop the event processor from processing future events.
static bool IsTransientFailure(network::HttpResult const& result) {
    auto status = http::status(result.Status());

    if (result.IsError() ||
        http::to_status_class(status) != http::status_class::client_error) {
        return true;
    }

    return status == http::status::bad_request ||
           status == http::status::request_timeout ||
           status == http::status::too_many_requests ||
           status == http::status::payload_too_large;
}

// Returns true if the request should be retried or not. Meant to be called
// when IsTransientFailure returns true.
static bool IsRetryable(network::HttpResult::StatusCode status) {
    return http::status(status) != http::status::payload_too_large;
}

static bool IsSuccess(network::HttpResult const& result) {
    return !result.IsError() &&
           http::to_status_class(http::status(result.Status())) ==
               http::status_class::successful;
}

void RequestWorker::OnDeliveryAttempt(network::HttpResult result,
                                      ResultCallback callback) {
    auto [next_state, action] = NextState(state_, result);

    LD_LOG(logger_, LogLevel::kDebug)
        << tag_ << state_ << " -> " << next_state << ", " << action << "";

    switch (action) {
        case Action::None:
            break;
        case Action::Reset:
            if (result.IsError()) {
                LD_LOG(logger_, LogLevel::kWarn)
                    << tag_ << "error posting " << batch_->Count()
                    << " event(s) (some events were dropped): "
                    << result.ErrorMessage().value_or("unknown IO error");
            } else {
                LD_LOG(logger_, LogLevel::kWarn)
                    << tag_ << "error posting " << batch_->Count()
                    << " event(s) (some events were dropped): "
                       "HTTP error "
                    << result.Status();
            }
            batch_.reset();
            break;
        case Action::NotifyPermanentFailure:
            LD_LOG(logger_, LogLevel::kWarn)
                << tag_ << "error posting " << batch_->Count()
                << " event(s) (giving up permanently): HTTP error "
                << result.Status();
            callback(batch_->Count(), result.Status());
            batch_.reset();
            break;
        case Action::ParseDateAndReset: {
            if (!date_header_locale_) {
                batch_.reset();
                break;
            }
            auto headers = result.Headers();
            if (auto date = headers.find("Date"); date != headers.end()) {
                if (auto server_time =
                        ParseDateHeader<std::chrono::system_clock>(
                            date->second, *date_header_locale_)) {
                    callback(batch_->Count(), *server_time);
                }
            }
            batch_.reset();
        } break;
        case Action::Retry:
            if (result.IsError()) {
                LD_LOG(logger_, LogLevel::kWarn)
                    << tag_ << "error posting " << batch_->Count()
                    << " event(s) (will retry): "
                    << result.ErrorMessage().value_or("unknown IO error");
            } else {
                LD_LOG(logger_, LogLevel::kWarn)
                    << tag_ << "error posting " << batch_->Count()
                    << " event(s) (will retry): HTTP error " << result.Status();
            }
            timer_.expires_from_now(retry_delay_);
            timer_.async_wait([this, callback](boost::system::error_code ec) {
                if (ec) {
                    return;
                }
                requester_.Request(
                    batch_->Request(),
                    [this, callback](network::HttpResult result) {
                        OnDeliveryAttempt(std::move(result),
                                          std::move(callback));
                    });
            });
            break;
    }

    state_ = next_state;
}

std::pair<State, Action> NextState(State state,
                                   network::HttpResult const& result) {
    std::optional<Action> action;
    std::optional<State> next_state;

    switch (state) {
        case State::Idle:
            return {state, Action::None};
        case State::PermanentlyFailed:
            return {state, Action::None};
        case State::FirstChance:
            if (IsSuccess(result)) {
                next_state = State::Idle;
                action = Action::ParseDateAndReset;
            } else if (IsTransientFailure(result)) {
                if (IsRetryable(result.Status())) {
                    next_state = State::SecondChance;
                    action = Action::Retry;
                } else {
                    next_state = State::Idle;
                    action = Action::Reset;
                }
            } else {
                next_state = State::PermanentlyFailed;
                action = Action::NotifyPermanentFailure;
            }
            break;
        case State::SecondChance:
            if (IsSuccess(result)) {
                next_state = State::Idle;
                action = Action::ParseDateAndReset;
            } else if (IsTransientFailure(result)) {
                // no more delivery attempts; payload is dropped.
                next_state = State::Idle;
                action = Action::Reset;
            } else {
                next_state = State::PermanentlyFailed;
                action = Action::NotifyPermanentFailure;
            }
            break;
    }

    assert(action.has_value() && "NextState must generate an action");
    assert(next_state.has_value() && "NextState must generate a new state");

    return {*next_state, *action};
}

std::ostream& operator<<(std::ostream& out, State const& s) {
    switch (s) {
        case State::Idle:
            out << "State::Idle";
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

}  // namespace launchdarkly::events
