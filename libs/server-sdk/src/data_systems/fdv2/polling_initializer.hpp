#pragma once

#include "../../data_interfaces/source/ifdv2_initializer.hpp"

#include <launchdarkly/async/promise.hpp>
#include <launchdarkly/data_model/selector.hpp>
#include <launchdarkly/fdv2_protocol_handler.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/network/asio_requester.hpp>
#include <launchdarkly/server_side/config/built/all_built.hpp>

#include <boost/asio/any_io_executor.hpp>

#include <optional>
#include <string>

namespace launchdarkly::server_side::data_systems {

/**
 * FDv2 polling initializer. Makes a single HTTP GET to the FDv2 polling
 * endpoint, parses the response via the FDv2 protocol state machine, and
 * returns the result. Implements IFDv2Initializer (async, one-shot).
 *
 * Threading model:
 *   Run() is called once from the orchestrator thread. It fires the HTTP
 *   request and returns a Future that resolves when the response arrives
 *   or Close() is called.
 *   Close() may be called from any thread, concurrently with Run().
 *   Destroying this object is not safe until the ASIO thread has been
 *   joined, because the HTTP response callback posted to the executor
 *   captures member variables.
 */
class FDv2PollingInitializer final : public data_interfaces::IFDv2Initializer {
   public:
    FDv2PollingInitializer(boost::asio::any_io_executor const& executor,
                           Logger const& logger,
                           config::built::ServiceEndpoints const& endpoints,
                           config::built::HttpProperties const& http_properties,
                           data_model::Selector selector,
                           std::optional<std::string> filter_key);

    async::Future<data_interfaces::FDv2SourceResult> Run() override;

    void Close() override;

    [[nodiscard]] std::string const& Identity() const override;

   private:
    data_interfaces::FDv2SourceResult HandlePollResult(
        network::HttpResult const& res);

    // Immutable after construction; safe to read from any thread.
    Logger const& logger_;
    network::HttpRequest const request_;

    // Mutable state accessed only from the ASIO thread (via the
    // requester_ callback).
    network::AsioRequester requester_;
    FDv2ProtocolHandler protocol_handler_;

    // Resolved when Close() is called, cancelling any outstanding Run().
    async::Promise<std::monostate> close_promise_;
};

}  // namespace launchdarkly::server_side::data_systems
