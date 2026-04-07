#pragma once

#include "../../data_interfaces/source/ifdv2_synchronizer.hpp"

#include <launchdarkly/fdv2_protocol_handler.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/network/asio_requester.hpp>
#include <launchdarkly/server_side/config/built/all_built.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/asio/steady_timer.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <optional>
#include <string>

namespace launchdarkly::server_side::data_systems {

/**
 * FDv2 polling synchronizer. Repeatedly polls the FDv2 polling endpoint at
 * a configurable interval. Implements IFDv2Synchronizer (blocking).
 *
 * The caller passes the current selector into each Next() call, allowing the
 * orchestrator to reflect applied changesets without any shared state.
 *
 * Threading model:
 *   Next() may be called from any single thread (the orchestrator thread).
 *   Close() may be called from any thread, concurrently with Next().
 *   Next() posts work to the ASIO executor and blocks on a future; the actual
 *   I/O and timer operations run on the ASIO thread. Destroying this object
 *   is not safe until the ASIO thread has been joined, because in-flight
 *   callbacks posted to the executor may still reference member variables.
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

    data_interfaces::FDv2SourceResult Next(
        std::chrono::milliseconds timeout,
        data_model::Selector selector) override;

    void Close() override;

    [[nodiscard]] std::string const& Identity() const override;

   private:
    // Called on the ASIO thread. Waits out any remaining inter-poll delay,
    // then calls DoPoll.
    void DoNext(std::chrono::milliseconds timeout,
                data_model::Selector selector,
                std::shared_ptr<std::promise<data_interfaces::FDv2SourceResult>>
                    promise);

    // Called on the ASIO thread. Fires the HTTP request and waits for the
    // response or timeout, then sets the promise.
    void DoPoll(std::chrono::time_point<std::chrono::steady_clock> deadline,
                data_model::Selector const& selector,
                std::shared_ptr<std::promise<data_interfaces::FDv2SourceResult>>
                    promise);

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

    // Mutable state accessed only from the ASIO thread (via DoNext/DoPoll).
    network::AsioRequester requester_;
    FDv2ProtocolHandler protocol_handler_;
    bool started_ = false;
    std::chrono::time_point<std::chrono::steady_clock> last_poll_start_;

    // Thread-safety-sensitive members. See threading model note in the class
    // doc above.

    // Accessed only from the ASIO thread. ASIO I/O objects are not thread-safe;
    // all operations on timer_ must run on the executor.
    boost::asio::steady_timer timer_;

    // Written by Close() from any thread; read by DoNext/DoPoll on the ASIO
    // thread. Must be atomic to avoid a data race.
    std::atomic<bool> closed_{false};

    // Accessed only from the ASIO thread. boost::asio::cancellation_signal is
    // not thread-safe: slot() (which registers a handler) and emit() (which
    // fires it) must not be called concurrently from different threads. Both
    // happen on the ASIO thread here: DoNext/DoPoll call slot() when initiating
    // a parallel_group, and Close() posts emit() to the executor.
    boost::asio::cancellation_signal cancel_signal_;
};

}  // namespace launchdarkly::server_side::data_systems
