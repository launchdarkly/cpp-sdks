#pragma once

#include "fdv2_request_config.hpp"
#include "ifdv2_synchronizer.hpp"

#include <launchdarkly/async/cancellation.hpp>
#include <launchdarkly/async/promise.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/network/requester.hpp>

#include <boost/asio/any_io_executor.hpp>

#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace launchdarkly::client_side::data_sources {

/**
 * Keeps flag data current by polling the FDv2 client polling endpoint on an
 * interval.
 *
 * Polls are rate limited. A poll that would come sooner than the interval
 * allows is delayed by the time remaining, so that repeated activations of
 * this synchronizer cannot produce a burst of requests.
 *
 * Threading model:
 *   Next() should only be called once at a time.
 *   Close() may be called concurrently with Next().
 *   This object may be safely destroyed once no call to Next() or Close() is
 *   in progress.
 */
class FDv2PollingSynchronizer final : public IFDv2Synchronizer {
   public:
    /**
     * @param executor Runs the HTTP requests and the interval timer.
     * @param logger Receives a description of any failure.
     * @param request_config How to reach the polling endpoint, and which
     * context to evaluate.
     * @param poll_interval How long to wait between polls. Clamped up to the
     * minimum the synchronizer permits.
     * @param last_poll The time of the most recent poll for this context, if
     * one is known, so that the first poll respects the interval too.
     */
    FDv2PollingSynchronizer(
        boost::asio::any_io_executor const& executor,
        Logger const& logger,
        FDv2RequestConfig const& request_config,
        std::chrono::seconds poll_interval,
        std::optional<std::chrono::steady_clock::time_point> last_poll);

    ~FDv2PollingSynchronizer() override;

    async::Future<FDv2SourceResult> Next(
        data_model::Selector selector) override;

    void Close() override;

    [[nodiscard]] std::string const& Identity() const override;

   private:
    // Any state that async callbacks may touch lives here, held by
    // shared_ptr so those callbacks can outlive the synchronizer.
    class State {
       public:
        State(Logger const& logger,
              boost::asio::any_io_executor const& executor,
              FDv2RequestConfig const& request_config,
              std::chrono::seconds poll_interval,
              std::optional<std::chrono::steady_clock::time_point> last_poll);

        /** Issues an async poll, resolving with the HTTP response. */
        [[nodiscard]] async::Future<network::HttpResult> Request(
            data_model::Selector const& selector) const;

        /** Interprets an HTTP response as a source result. */
        FDv2SourceResult HandlePollResult(network::HttpResult const& res) const;

        /**
         * Returns a Future that resolves when the interval permits the next
         * poll, or early with false if the token is cancelled first.
         */
        [[nodiscard]] async::Future<bool> AwaitNextPoll(
            async::CancellationToken token);

        /** Records that a poll has started, for interval scheduling. */
        void RecordPollStarted();

       private:
        // Logger is itself thread-safe.
        Logger const logger_;

        // Immutable state.
        std::chrono::seconds const poll_interval_;
        FDv2RequestConfig const request_config_;
        network::Requester const requester_;
        boost::asio::any_io_executor const executor_;

        std::mutex mutex_;
        // Protected by mutex_.
        std::optional<std::chrono::steady_clock::time_point> last_poll_start_;
    };

    /**
     * Waits for the poll interval, then delegates to DoPoll. Resolves with
     * Shutdown if closed before the next poll begins.
     */
    static async::Future<FDv2SourceResult> DoNext(
        std::shared_ptr<State> state,
        async::Future<std::monostate> closed,
        data_model::Selector selector);

    /**
     * Issues a single poll and returns the result. Resolves with Shutdown if
     * closed before the request completes.
     */
    static async::Future<FDv2SourceResult> DoPoll(
        std::shared_ptr<State> state,
        async::Future<std::monostate> closed,
        data_model::Selector const& selector);

    // Resolved by Close() or on destruction, cancelling any outstanding
    // Next() call.
    async::Promise<std::monostate> close_promise_;

    // Shared with async callbacks.
    std::shared_ptr<State> state_;
};

}  // namespace launchdarkly::client_side::data_sources
