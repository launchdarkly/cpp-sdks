#include "events/detail/request_worker.hpp"
#include "events/detail/parse_date_header.hpp"

namespace launchdarkly::events::detail {

RequestWorker::RequestWorker(boost::asio::any_io_executor io,
                             std::chrono::milliseconds retry_after,
                             Logger& logger)
    : timer_(io),
      retry_delay_(retry_after),
      state_(State::Available),
      requester_(timer_.get_executor()),
      batch_(std::nullopt),
      logger_(logger) {}

bool RequestWorker::Available() const {
    return state_ == State::Available;
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

void RequestWorker::OnDeliveryAttempt(network::detail::HttpResult result,
                                      ResultCallback callback) {
    auto [next_state, action] = NextState(state_, result);

    LD_LOG(logger_, LogLevel::kDebug) << "flush-worker: " << state_ << " -> "
                                      << next_state << ", " << action << "";

    switch (action) {
        case Action::None:
            break;
        case Action::Reset:
            if (result.IsError()) {
                LD_LOG(logger_, LogLevel::kWarn)
                    << "error posting " << batch_->Count()
                    << " event(s) (some events were dropped): "
                    << result.ErrorMessage().value_or("unknown IO error");
            } else {
                LD_LOG(logger_, LogLevel::kWarn)
                    << "error posting " << batch_->Count()
                    << " event(s) (some events were dropped): "
                       "HTTP error "
                    << result.Status();
            }
            batch_.reset();
            break;
        case Action::NotifyPermanentFailure:
            LD_LOG(logger_, LogLevel::kWarn)
                << "error posting " << batch_->Count()
                << " event(s) (giving up permanently): HTTP error "
                << result.Status();
            callback(batch_->Count(), result.Status());
            batch_.reset();
            break;
        case Action::ParseDateAndReset: {
            auto headers = result.Headers();
            if (auto date = headers.find("Date"); date != headers.end()) {
                if (auto server_time =
                        ParseDateHeader<std::chrono::system_clock>(
                            date->second)) {
                    callback(batch_->Count(), *server_time);
                }
            }
            batch_.reset();
        } break;
        case Action::Retry:
            if (result.IsError()) {
                LD_LOG(logger_, LogLevel::kWarn)
                    << "error posting " << batch_->Count()
                    << " event(s) (will retry): "
                    << result.ErrorMessage().value_or("unknown IO error");
            } else {
                LD_LOG(logger_, LogLevel::kWarn)
                    << "error posting " << batch_->Count()
                    << " event(s) (will retry): HTTP error " << result.Status();
            }
            timer_.expires_from_now(retry_delay_);
            timer_.async_wait([this, callback](boost::system::error_code ec) {
                if (ec) {
                    return;
                }
                requester_.Request(
                    batch_->Request(),
                    [this, callback](network::detail::HttpResult result) {
                        OnDeliveryAttempt(std::move(result),
                                          std::move(callback));
                    });
            });
            break;
    }

    state_ = next_state;
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
