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
 * a configurable interval. Implements IFDv2Synchronizer (async).
 *
 * The caller passes the current selector into each Next() call, allowing the
 * orchestrator to reflect applied changesets without any shared state.
 *
 * Threading model:
 *   Next() may be called from any thread. Close() may be called from any
 *   thread, concurrently with Next(). Destroying this object is not safe
 *   until all in-flight callbacks posted to the executor have completed,
 *   because they may still reference member variables.
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
    // Fires the HTTP request and races it against the timeout and close
    // signal. Returns a Future that resolves with the result.
    async::Future<data_interfaces::FDv2SourceResult> DoPoll(
        std::chrono::time_point<std::chrono::steady_clock> deadline,
        data_model::Selector const& selector);

    network::HttpRequest MakeRequest(
        data_model::Selector const& selector) const;
    data_interfaces::FDv2SourceResult HandlePollResult(
        network::HttpResult const& res);

    // Immutable after construction; safe to read from any thread.
    Logger const& logger_;
    config::built::ServiceEndpoints const& endpoints_;
    config::built::HttpProperties const& http_properties_;
    std::optional<std::string> const filter_key_;
    std::chrono::seconds const poll_interval_;

    // Used to construct Delay() timers. any_io_executor is a value type safe
    // to use from any thread.
    boost::asio::any_io_executor executor_;

    // Accessed only from the orchestrator thread (via DoNext/DoPoll).
    network::AsioRequester requester_;
    bool started_ = false;
    std::chrono::time_point<std::chrono::steady_clock> last_poll_start_;

    // Protects protocol_handler_, which is reset in DoPoll and read in the
    // Then continuation when an HTTP response arrives.
    std::mutex protocol_handler_mutex_;
    FDv2ProtocolHandler protocol_handler_;

    // Written by Close() from any thread; read by DoNext/DoPoll on the ASIO
    // thread. Must be atomic to avoid a data race.
    std::atomic<bool> closed_{false};

    // Resolved when Close() is called, cancelling any outstanding calls to
    // Next().
    async::Promise<std::monostate> close_promise_;
};

}  // namespace launchdarkly::server_side::data_systems
