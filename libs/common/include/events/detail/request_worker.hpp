#pragma once
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <functional>
#include <tuple>
#include "logger.hpp"
#include "network/detail/asio_requester.hpp"
#include "network/detail/http_requester.hpp"
namespace launchdarkly::events::detail {

enum class State {
    Unknown = 0,
    /* Worker is ready for a new job. */
    Available = 1,
    /* Worker is performing the 1st delivery. */
    FirstChance = 2,
    /* Worker is performing the 2nd (final) delivery. */
    SecondChance = 3,
    /* Worker has encountered an error that cannot be recovered from. */
    PermanentlyFailed = 4,
};

std::ostream& operator<<(std::ostream& out, State const& s);

enum class Action {
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
    using ServerTimeCallback =
        std::function<void(std::chrono::system_clock::time_point server_time)>;

    using PermanentFailureCallback =
        std::function<void(network::detail::HttpResult::StatusCode)>;

    /**
     * Constructs a new RequestWorker.
     * @param io The executor used to perform HTTP requests and retry
     * operations.
     * @param retry_after How long to wait after a recoverable failure before
     * trying to deliver events again.
     * @param logger Logger for debug messages.
     * @param server_time_cb Callback invoked whenever the worker encounters a
     * Date header on an HTTP response that was deemed successful. May be
     * invoked zero or more times; never invoked after permanent_failure_cb.
     * @param permanent_failure_cb Callback invoked whenever the worker
     * encounters permanent failure, meaning it is unable to perform any future
     * work. Invoked exactly once.
     */
    RequestWorker(boost::asio::any_io_executor io,
                  std::chrono::milliseconds retry_after,
                  ServerTimeCallback server_time_cb,
                  Logger& logger);

    /**
     * Returns true if the worker is available for delivery. Not thread-safe.
     */
    bool Available() const;

    /**
     * Passes an HttpRequest to the worker for delivery. The delivery may be
     * retried exactly once if the failure is recoverable.
     *
     * Not thread-safe.
     *
     * @param request Request to deliver.
     */
    template <typename CompletionToken>
    auto AsyncDeliver(network::detail::HttpRequest request,
                      CompletionToken&& token) {
        namespace asio = boost::asio;
        namespace system = boost::system;

        using Sig = void(network::detail::HttpResult::StatusCode);
        using Result = asio::async_result<std::decay_t<CompletionToken>, Sig>;
        using Handler = typename Result::completion_handler_type;

        Handler handler(std::forward<decltype(token)>(token));
        Result result(handler);

        state_ = State::FirstChance;
        request_ = std::move(request);
        requester_.Request(
            *request_, [this, handler](network::detail::HttpResult result) {
                OnDeliveryAttempt(std::move(result), std::move(handler));
            });
        return result.get();
    }

   private:
    /* Used to wait a specific amount of time after a failed request before
     * trying again. */
    boost::asio::steady_timer timer_;

    /* How long to wait before trying again. */
    std::chrono::milliseconds retry_delay_;

    /* Current state of the RequestWorker. */
    State state_;

    /* Component used to perform HTTP operations. */
    network::detail::AsioRequester requester_;

    /* Current request; only present if AsyncDeliver was called and
     * request is in-flight or a retry is taking place. */
    std::optional<network::detail::HttpRequest> request_;

    /* Invoked after parsing the Date header on HTTP responses, which
     * may or may not be present. */
    ServerTimeCallback server_time_cb_;

    /* Invoked after determining that an HTTP failure is permanent. */
    PermanentFailureCallback permanent_failure_cb_;

    /* Used for debug logging. */
    Logger& logger_;

    /* Completion handler invoked from the AsioRequester. */
    void OnDeliveryAttempt(network::detail::HttpResult request,
                           PermanentFailureCallback cb);
};

}  // namespace launchdarkly::events::detail
