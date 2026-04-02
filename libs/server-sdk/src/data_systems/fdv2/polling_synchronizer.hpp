#pragma once

#include "../../data_interfaces/source/ifdv2_synchronizer.hpp"

#include <launchdarkly/fdv2_protocol_handler.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/network/asio_requester.hpp>
#include <launchdarkly/server_side/config/built/all_built.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/steady_timer.hpp>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>

namespace launchdarkly::server_side::data_systems {

/**
 * FDv2 polling synchronizer. Repeatedly polls the FDv2 polling endpoint at
 * a configurable interval. Implements IFDv2Synchronizer (blocking).
 *
 * The caller passes the current selector into each Next() call, allowing the
 * orchestrator to reflect applied changesets without any shared state.
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
    void DoPoll(data_model::Selector selector);
    network::HttpRequest MakeRequest(data_model::Selector const& selector) const;
    data_interfaces::FDv2SourceResult HandlePollResult(
        network::HttpResult const& res);

    Logger const& logger_;
    network::AsioRequester requester_;
    config::built::ServiceEndpoints const& endpoints_;
    config::built::HttpProperties const& http_properties_;
    std::optional<std::string> filter_key_;
    std::chrono::seconds poll_interval_;
    boost::asio::steady_timer timer_;
    FDv2ProtocolHandler protocol_handler_;

    bool started_ = false;
    std::chrono::time_point<std::chrono::steady_clock> last_poll_start_;

    std::mutex mutex_;
    std::condition_variable cv_;
    std::optional<network::HttpResult> result_;  // guarded by mutex_
    bool closed_ = false;                        // guarded by mutex_
};

}  // namespace launchdarkly::server_side::data_systems
