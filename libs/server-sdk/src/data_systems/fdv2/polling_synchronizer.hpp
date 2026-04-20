#pragma once

#include "../../data_interfaces/source/ifdv2_synchronizer.hpp"

#include <launchdarkly/async/cancellation.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/network/asio_requester.hpp>
#include <launchdarkly/server_side/config/built/all_built.hpp>

#include <boost/asio/any_io_executor.hpp>

#include <chrono>
#include <mutex>
#include <optional>
#include <string>

namespace launchdarkly::server_side::data_systems {

/**
 * FDv2 polling synchronizer. Repeatedly polls the FDv2 polling endpoint at
 * a configurable interval.
 *
 * The caller passes the current selector into each Next() call, allowing the
 * orchestrator to reflect applied changesets without any shared state.
 *
 * Threading model:
 *   Next() should only be called once at a time.
 *   Close() may be called concurrently with Next().
 *   This object may be safely destroyed once no call to Next() or Close()
 *   is in progress.
 */
class FDv2PollingSynchronizer final
    : public data_interfaces::IFDv2Synchronizer {
   public:
    /**
     * Constructs a synchronizer that polls at the given interval.
     * If filter_key is present, only the specified payload filter is requested.
     */
    FDv2PollingSynchronizer(
        boost::asio::any_io_executor const& executor,
        Logger const& logger,
        config::built::ServiceEndpoints const& endpoints,
        config::built::HttpProperties const& http_properties,
        std::optional<std::string> filter_key,
        std::chrono::seconds poll_interval);

    ~FDv2PollingSynchronizer() override;

    async::Future<data_interfaces::FDv2SourceResult> Next(
        std::chrono::milliseconds timeout,
        data_model::Selector selector) override;

    void Close() override;

    [[nodiscard]] std::string const& Identity() const override;

   private:
    // Any state that may be accessed by async callbacks needs to be inside this
    // class, managed by a shared_ptr. All mutable members are guarded by the
    // mutex.
    class State {
       public:
        State(Logger logger,
              boost::asio::any_io_executor const& executor,
              std::chrono::seconds poll_interval,
              config::built::ServiceEndpoints const& endpoints,
              config::built::HttpProperties const& http_properties,
              std::optional<std::string> filter_key);

        /** Issues an async HTTP poll request and returns a Future resolving
         * with the result. */
        async::Future<network::HttpResult> Request(
            data_model::Selector const& selector) const;

        /** Interprets an HTTP response as a source result. */
        data_interfaces::FDv2SourceResult HandlePollResult(
            network::HttpResult const& res);

        /** Returns a Future that resolves when it is time to start the next
         * poll. If a token is provided and cancelled before the delay elapses,
         * the future resolves early with false. */
        async::Future<bool> CreatePollDelayFuture(
            async::CancellationToken token = {});

        /** Records that a poll has started, for interval scheduling. */
        void RecordPollStarted();

        /** Returns a Future that resolves after the given duration. If a token
         * is provided and cancelled before the duration elapses, the future
         * resolves early with false. */
        async::Future<bool> Delay(std::chrono::nanoseconds duration,
                                  async::CancellationToken token = {});

       private:
        // Logger is itself thread-safe.
        Logger logger_;

        // Immutable state
        std::chrono::seconds const poll_interval_;
        config::built::ServiceEndpoints const endpoints_;
        config::built::HttpProperties const http_properties_;
        std::optional<std::string> const filter_key_;
        network::AsioRequester const requester_;
        boost::asio::any_io_executor const executor_;

        // Mutable state, guarded by mutex_.
        std::mutex mutex_;
        std::optional<std::chrono::time_point<std::chrono::steady_clock>>
            last_poll_start_;
    };

    /**
     * Waits for the poll interval, then delegates to DoPoll.
     * Resolves with Shutdown if closed, or Timeout if the timeout expires
     * first.
     */
    static async::Future<data_interfaces::FDv2SourceResult> DoNext(
        std::shared_ptr<State> state,
        async::Future<std::monostate> closed,
        std::chrono::milliseconds timeout,
        data_model::Selector selector);

    /**
     * Issues a single HTTP poll request and returns the result.
     * Resolves with Shutdown if closed, or Timeout if timeout_deadline passes
     * first.
     */
    static async::Future<data_interfaces::FDv2SourceResult> DoPoll(
        std::shared_ptr<State> state,
        async::Future<std::monostate> closed,
        std::chrono::time_point<std::chrono::steady_clock> timeout_deadline,
        data_model::Selector const& selector);

    // Resolved by Close() or on destruction, cancelling any outstanding Next()
    // calls.
    async::Promise<std::monostate> close_promise_;

    // Shared with async callbacks.
    std::shared_ptr<State> state_;
};

}  // namespace launchdarkly::server_side::data_systems
