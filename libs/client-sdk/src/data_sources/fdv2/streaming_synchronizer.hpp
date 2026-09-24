#pragma once

#include "fdv2_request_config.hpp"
#include "ifdv2_synchronizer.hpp"

#include <launchdarkly/async/promise.hpp>
#include <launchdarkly/fdv2_protocol_handler.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/network/requester.hpp>
#include <launchdarkly/sse/client.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/url/url.hpp>

#include <chrono>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace launchdarkly::client_side::data_sources {

class FDv2StreamingSynchronizerTestPeer;

/**
 * Keeps flag data current over a long-lived connection to the FDv2 client
 * streaming endpoint, turning the push-based event stream into the pull-based
 * IFDv2Synchronizer::Next() interface.
 *
 * Threading model:
 *   Next() should only be called once at a time.
 *   Close() may be called concurrently with Next().
 *   This object may be safely destroyed once no call to Next() or Close() is
 *   in progress.
 */
class FDv2StreamingSynchronizer final : public IFDv2Synchronizer {
    friend class FDv2StreamingSynchronizerTestPeer;

   public:
    /**
     * @param executor Runs the stream, the ping-triggered polls, and the
     * reconnection backoff.
     * @param logger Receives a description of any failure.
     * @param stream_config How to reach the streaming endpoint, and which
     * context to evaluate.
     * @param poll_config Where to poll in answer to a `ping` event. Must
     * describe the same context as stream_config.
     * @param initial_reconnect_delay Where the reconnection backoff starts.
     */
    FDv2StreamingSynchronizer(
        boost::asio::any_io_executor const& executor,
        Logger const& logger,
        FDv2RequestConfig const& stream_config,
        FDv2RequestConfig const& poll_config,
        std::chrono::milliseconds initial_reconnect_delay);

    ~FDv2StreamingSynchronizer() override;

    async::Future<FDv2SourceResult> Next(
        data_model::Selector selector) override;

    void Close() override;

    [[nodiscard]] std::string const& Identity() const override;

   private:
    // Any state that async SSE callbacks may touch lives here, held by
    // shared_ptr so those callbacks can outlive the synchronizer.
    class State : public std::enable_shared_from_this<State> {
        friend class FDv2StreamingSynchronizerTestPeer;

       public:
        State(Logger const& logger,
              boost::asio::any_io_executor const& executor,
              FDv2RequestConfig const& stream_config,
              FDv2RequestConfig const& poll_config,
              std::chrono::milliseconds initial_reconnect_delay);

        /**
         * Records the selector to send on the next connection attempt, starts
         * the stream if it is not already running, and returns a Future
         * resolving with the next result.
         *
         * If a result is already buffered the Future is resolved
         * immediately. Otherwise it resolves when the next event arrives.
         *
         * @param self The shared_ptr owning this State, used to form the weak
         * references the SSE callbacks capture.
         */
        async::Future<FDv2SourceResult> Next(
            data_model::Selector const& selector,
            std::shared_ptr<State> self);

        /**
         * Abandons an outstanding Next() call without delivering a result.
         * Any result that arrives afterwards is buffered for the next call.
         */
        void ClearPendingPromise();

        /**
         * Marks the State closed and shuts down the stream if one was
         * started. After Shutdown returns, no new stream can start.
         * Idempotent.
         */
        void Shutdown();

       private:
        using HttpRequest =
            boost::beast::http::request<boost::beast::http::string_body>;
        using HttpResponseHeader = boost::beast::http::response_header<>;

        /** Starts the stream if it is not already running. */
        void EnsureStarted(data_model::Selector const& selector,
                           std::shared_ptr<State> self);

        /**
         * Delivers a result to the caller of Next(), or buffers it if no
         * caller is waiting.
         */
        void Notify(FDv2SourceResult result);

        /**
         * Issues the poll a `ping` event calls for, delivering its result
         * when it arrives. A ping received while an answering poll is still
         * in flight is dropped, so that a burst of pings cannot pile up
         * requests.
         */
        void PollForPing(std::shared_ptr<State> self);

        // SSE client callbacks.
        void OnConnect(HttpRequest* req);
        void OnResponse(HttpResponseHeader const& headers);
        void OnEvent(sse::Event const& event);
        void OnError(sse::Error const& error);

        // Logger is itself thread-safe.
        Logger const logger_;

        // Immutable state.
        FDv2RequestConfig const stream_config_;
        FDv2RequestConfig const poll_config_;
        std::chrono::milliseconds const initial_reconnect_delay_;
        boost::asio::any_io_executor const executor_;
        network::Requester const requester_;

        // Touched only from SSE callbacks, which all run on the same strand.
        // No lock required.
        FDv2ProtocolHandler protocol_handler_;

        std::mutex mutex_;
        // All protected by mutex_.
        bool started_ = false;
        bool closed_ = false;
        bool ping_poll_in_flight_ = false;
        // From the most recent stream response.
        std::optional<FDv1FallbackDirective> latest_fdv1_fallback_;
        std::optional<std::string> latest_environment_id_;
        data_model::Selector latest_selector_;
        std::optional<boost::urls::url> base_url_;
        std::shared_ptr<sse::Client> sse_client_;
        std::optional<async::Promise<FDv2SourceResult>> pending_promise_;
        std::deque<FDv2SourceResult> result_queue_;
    };

    // Resolved by Close() or on destruction, cancelling any outstanding
    // Next() call.
    async::Promise<std::monostate> close_promise_;

    // Shared with async SSE callbacks.
    std::shared_ptr<State> state_;
};

}  // namespace launchdarkly::client_side::data_sources
