#pragma once

#include "../../data_interfaces/source/ifdv2_synchronizer.hpp"

#include <launchdarkly/async/cancellation.hpp>
#include <launchdarkly/async/promise.hpp>
#include <launchdarkly/fdv2_protocol_handler.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/server_side/config/built/all_built.hpp>
#include <launchdarkly/sse/client.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/url/url.hpp>

#include <chrono>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace launchdarkly::server_side::data_systems {

class FDv2StreamingSynchronizerTestPeer;

/**
 * FDv2 streaming synchronizer. Opens an SSE connection to the FDv2 streaming
 * endpoint and translates the push-based event stream into the pull-based
 * IFDv2Synchronizer::Next() interface.
 *
 * Threading model:
 *   Next() should only be called once at a time.
 *   Close() may be called concurrently with Next().
 *   This object may be safely destroyed once no call to Next() or Close()
 *   is in progress.
 */
class FDv2StreamingSynchronizer final
    : public data_interfaces::IFDv2Synchronizer {
    friend class FDv2StreamingSynchronizerTestPeer;

   public:
    /**
     * Constructs a synchronizer that streams from the FDv2 streaming endpoint.
     * If filter_key is present, only the specified payload filter is requested.
     */
    FDv2StreamingSynchronizer(
        boost::asio::any_io_executor const& executor,
        Logger const& logger,
        config::built::ServiceEndpoints const& endpoints,
        config::built::HttpProperties const& http_properties,
        std::optional<std::string> filter_key,
        std::chrono::milliseconds initial_reconnect_delay);

    ~FDv2StreamingSynchronizer() override;

    async::Future<data_interfaces::FDv2SourceResult> Next(
        std::chrono::milliseconds timeout,
        data_model::Selector selector) override;

    void Close() override;

    [[nodiscard]] std::string const& Identity() const override;

   private:
    // Any state that may be accessed by async SSE callbacks needs to be inside
    // this class, managed by a shared_ptr. All mutable members are guarded by
    // the mutex.
    class State {
        friend class FDv2StreamingSynchronizerTestPeer;

       public:
        State(Logger logger,
              boost::asio::any_io_executor const& executor,
              config::built::ServiceEndpoints const& endpoints,
              config::built::HttpProperties const& http_properties,
              std::optional<std::string> filter_key,
              std::chrono::milliseconds initial_reconnect_delay);

        /**
         * Updates the stored selector, starts the SSE client if not already
         * running, and returns a Future resolving with the next result.
         *
         * If a buffered result is already available, the Future is resolved
         * immediately. Otherwise it resolves when the next event arrives.
         *
         * The self parameter must be the shared_ptr that owns this State,
         * used to form weak_ptr captures in SSE callbacks.
         */
        async::Future<data_interfaces::FDv2SourceResult> Next(
            data_model::Selector const& selector,
            std::shared_ptr<State> self);

        /**
         * Cancels an outstanding Next() call without delivering a result.
         * Any result that subsequently arrives will be buffered for the next
         * Next() call.
         */
        void ClearPendingPromise();

        /**
         * Marks the State as closed and shuts down the SSE client if one has
         * been built. After Shutdown returns, EnsureStarted is guaranteed not
         * to start a new client. Idempotent.
         */
        void Shutdown();

        /**
         * Returns a Future that resolves after the given duration. Resolves
         * early with false if the token is cancelled before the duration
         * elapses.
         */
        async::Future<bool> Delay(std::chrono::milliseconds duration,
                                  async::CancellationToken token = {});

       private:
        using HttpRequest =
            boost::beast::http::request<boost::beast::http::string_body>;

        /**
         * Starts the SSE client if not already running, and updates the stored
         * selector so that the on-connect hook will use it for the next
         * connection attempt.
         */
        void EnsureStarted(data_model::Selector const& selector,
                           std::shared_ptr<State> self);

        /**
         * Delivers a result to the caller of Next(), or buffers it if no
         * caller is waiting.
         */
        void Notify(data_interfaces::FDv2SourceResult result);

        /**
         * Per-connect hook for the SSE client. Overwrites the request target
         * with the streaming path plus query parameters built from the latest
         * stored selector and the (immutable) filter key.
         */
        void OnConnect(HttpRequest* req);

        void OnEvent(sse::Event const& event);
        void OnError(sse::Error const& error);

        // Logger is itself thread-safe.
        Logger logger_;

        // Immutable state
        config::built::ServiceEndpoints const endpoints_;
        config::built::HttpProperties const http_properties_;
        std::optional<std::string> const filter_key_;
        std::chrono::milliseconds const initial_reconnect_delay_;
        boost::asio::any_io_executor const executor_;

        // Touched only from SSE callbacks, which all run on the same strand.
        // No lock required.
        FDv2ProtocolHandler protocol_handler_;

        // Mutable state, all guarded by mutex_.
        std::mutex mutex_;
        bool started_ = false;
        bool closed_ = false;
        data_model::Selector latest_selector_;
        std::optional<boost::urls::url> base_url_;
        std::shared_ptr<sse::Client> sse_client_;
        std::optional<async::Promise<data_interfaces::FDv2SourceResult>>
            pending_promise_;
        std::deque<data_interfaces::FDv2SourceResult> result_queue_;
    };

    // Resolved by Close() or on destruction, cancelling any outstanding Next().
    async::Promise<std::monostate> close_promise_;

    // Shared with async SSE callbacks.
    std::shared_ptr<State> state_;
};

}  // namespace launchdarkly::server_side::data_systems
