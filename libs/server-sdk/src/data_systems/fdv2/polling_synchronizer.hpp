#pragma once

#include "../../data_interfaces/source/ifdv2_synchronizer.hpp"

#include <launchdarkly/async/timer.hpp>
#include <launchdarkly/fdv2_protocol_handler.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/network/asio_requester.hpp>
#include <launchdarkly/server_side/config/built/all_built.hpp>

#include <boost/asio/any_io_executor.hpp>

#include <atomic>
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
 *   Next() may be called from any thread. Close() may be called from any
 *   thread, concurrently with Next().
 */
class FDv2PollingSynchronizer final
    : public data_interfaces::IFDv2Synchronizer {
   public:
    FDv2PollingSynchronizer(
        boost::asio::any_io_executor const& executor,
        Logger const& logger,
        config::built::ServiceEndpoints const& endpoints,
        config::built::HttpProperties const& http_properties,
        std::optional<std::string> filter_key,
        std::chrono::seconds poll_interval);

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
              std::optional<std::string> filter_key,
              async::Future<std::monostate> closed);

        async::Future<network::HttpResult> Request(
            data_model::Selector const& selector) const;

        data_interfaces::FDv2SourceResult HandlePollResult(
            network::HttpResult const& res);

        async::Future<bool> CreatePollDelayFuture();
        void RecordPollStarted();

        template <typename Rep, typename Period>
        async::Future<bool> Delay(std::chrono::duration<Rep, Period> duration) {
            return async::Delay(executor_, duration);
        }

       private:
        // TODO: Is the logger thread-safe?
        Logger logger_;

        // Immutable state
        std::chrono::seconds const poll_interval_;
        config::built::ServiceEndpoints const endpoints_;
        config::built::HttpProperties const http_properties_;
        std::optional<std::string> const filter_key_;
        network::AsioRequester const requester_;

        // Used to construct Delay() timers. This is a thread-safe value type.
        boost::asio::any_io_executor executor_;

        // Mutable state, guarded by mutex_.
        std::mutex mutex_;
        bool started_ = false;
        std::chrono::time_point<std::chrono::steady_clock> last_poll_start_;
    };

    static async::Future<data_interfaces::FDv2SourceResult> DoNext(
        std::shared_ptr<State> state,
        async::Future<std::monostate> closed,
        std::chrono::milliseconds timeout,
        data_model::Selector const& selector);

    static async::Future<data_interfaces::FDv2SourceResult> DoPoll(
        std::shared_ptr<State> state,
        async::Future<std::monostate> closed,
        std::chrono::time_point<std::chrono::steady_clock> timeout_deadline,
        data_model::Selector const& selector);

    // Resolved by Close(), cancelling any outstanding Next() calls.
    async::Promise<std::monostate> close_promise_;

    std::shared_ptr<State> state_;
};

}  // namespace launchdarkly::server_side::data_systems
