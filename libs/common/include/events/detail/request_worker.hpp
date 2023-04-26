#pragma once
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <functional>
#include <tuple>
#include "events/detail/event_batch.hpp"
#include "logger.hpp"
#include "network/detail/asio_requester.hpp"
#include "network/detail/http_requester.hpp"

namespace launchdarkly::events::detail {

enum class State {
    /* Worker is ready for a new job. */
    Idle = 1,
    /* Worker is performing the 1st delivery. */
    FirstChance = 2,
    /* Worker is performing the 2nd (final) delivery. */
    SecondChance = 3,
    /* Worker has encountered an error that cannot be recovered from. */
    PermanentlyFailed = 4,
};

std::ostream& operator<<(std::ostream& out, State const& s);

enum class Action {
    /* No action necessary. */
    None = 0,
    /* Free the current request. */
    Reset = 1,
    /* Attempt to parse a Date header out of a request, and then free it. */
    ParseDateAndReset = 2,
    /* Wait and then retry delivery of the same request. */
    Retry = 3,
    /* Invoke the permanent failure callback; free current request. */
    NotifyPermanentFailure = 4,
};

std::ostream& operator<<(std::ostream& out, Action const& s);

/**
 * Computes the next (state, action) pair from an existing state and an HTTP
 * result.
 * @param state Current state.
 * @param result HTTP result.
 * @return Next state + action to take.
 */
std::pair<State, Action> NextState(State,
                                   network::detail::HttpResult const& result);

/**
 * RequestWorker is responsible for initiating HTTP requests to deliver
 * event payloads to LaunchDarkly. It will attempt re-delivery once if
 * an HTTP failure is deemed recoverable.
 *
 * RequestWorkers are meant to be instantiated once and then used repeatedly
 * as they become available.
 */
class RequestWorker {
   public:
    /*
     * A delivery request can resolve as a PermanentFailureResult,
     * meaning the request could not be delivered at all. In this case, the HTTP
     * status code is made available for inspection.
     */
    using PermanentFailureResult = network::detail::HttpResult::StatusCode;

    /*
     * A delivery request can resolve as a ServerTimeResult, meaning the request
     * succeeded *and* a valid server timestamp was received.
     */
    using ServerTimeResult = std::chrono::system_clock::time_point;

    using DeliveryResult =
        std::variant<PermanentFailureResult, ServerTimeResult>;

    /*
     * A request made with AsyncDeliver may result in invocation of the
     * provided ResultCallback in exactly two cases:
     * - The delivery permanently failed. The callback will be invoked
     *   with the number of events in the batch, and a PermanentFailureResult.
     * - The delivery succeeded and the server returned a valid timestamp. The
     * callback will be invoked with the number of events in the batch, and a
     * ServerTimeResult.
     */
    using ResultCallback = std::function<void(std::size_t, DeliveryResult)>;

    /**
     * Constructs a new RequestWorker.
     * @param io The executor used to perform HTTP requests and retry
     * operations.
     * @param retry_after How long to wait after a recoverable failure before
     * trying to deliver events again.
     * @param logger Logger.
     */
    RequestWorker(boost::asio::any_io_executor io,
                  std::chrono::milliseconds retry_after,
                  Logger& logger);

    /**
     * Returns true if the worker is available for delivery.
     */
    bool Available() const;

    /**
     * Passes an EventBatch to the worker for delivery. The delivery may be
     * retried exactly once if the failure is recoverable.
     *
     * Completion is not guaranteed; see documentation on ResultCallback for
     * info.
     *
     * @param batch The events to deliver.
     */
    template <typename CompletionToken>
    auto AsyncDeliver(EventBatch batch, CompletionToken&& token) {
        namespace asio = boost::asio;
        namespace system = boost::system;

        using Sig = void(DeliveryResult);
        using Result = asio::async_result<std::decay_t<CompletionToken>, Sig>;
        using Handler = typename Result::completion_handler_type;

        Handler handler(std::forward<decltype(token)>(token));
        Result result(handler);

        state_ = State::FirstChance;
        batch_ = std::move(batch);

        LD_LOG(logger_, LogLevel::kDebug)
            << "posting " << batch_->Count() << " events(s) to "
            << batch_->Target() << " with payload: "
            << batch_->Request().Body().value_or("(no body)");

        requester_.Request(batch_->Request(),
                           [this, handler](network::detail::HttpResult result) {
                               OnDeliveryAttempt(std::move(result),
                                                 std::move(handler));
                           });
        return result.get();
    }

   private:
    /* Used to wait a specific amount of time after a failed request before
     * trying again. */
    boost::asio::steady_timer timer_;

    /* How long to wait before trying again. */
    std::chrono::milliseconds retry_delay_;

    /* Current state of the worker. */
    State state_;

    /* Component used to perform HTTP operations. */
    network::detail::AsioRequester requester_;

    /* Current event batch; only present if AsyncDeliver was called and
     * request is in-flight or a retry is taking place. */
    std::optional<EventBatch> batch_;

    Logger& logger_;

    void OnDeliveryAttempt(network::detail::HttpResult request,
                           ResultCallback cb);
};

}  // namespace launchdarkly::events::detail
