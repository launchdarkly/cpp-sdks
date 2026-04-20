#pragma once

#include "../../data_interfaces/source/ifdv2_initializer.hpp"

#include <launchdarkly/async/promise.hpp>
#include <launchdarkly/data_model/selector.hpp>
#include <launchdarkly/fdv2_protocol_handler.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/network/asio_requester.hpp>
#include <launchdarkly/server_side/config/built/all_built.hpp>

#include <boost/asio/any_io_executor.hpp>

#include <memory>
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
 *   This object may be safely destroyed once no call to Run() or Close()
 *   is in progress.
 */
class FDv2PollingInitializer final : public data_interfaces::IFDv2Initializer {
   public:
    FDv2PollingInitializer(boost::asio::any_io_executor const& executor,
                           Logger const& logger,
                           config::built::ServiceEndpoints const& endpoints,
                           config::built::HttpProperties const& http_properties,
                           data_model::Selector selector,
                           std::optional<std::string> filter_key);

    ~FDv2PollingInitializer() override;

    async::Future<data_interfaces::FDv2SourceResult> Run() override;

    void Close() override;

    [[nodiscard]] std::string const& Identity() const override;

   private:
    // State needed by async callbacks. Shared so callbacks can safely
    // outlive 'this'.
    struct State {
        Logger logger;
        FDv2ProtocolHandler protocol_handler;

        explicit State(Logger logger) : logger(std::move(logger)) {}
    };

    static data_interfaces::FDv2SourceResult HandlePollResult(
        std::shared_ptr<State> state,
        network::HttpResult const& res);

    // Immutable after construction; safe to read from any thread.
    network::HttpRequest const request_;

    // Accessed only synchronously from Run().
    network::AsioRequester requester_;

    // Resolved when Close() is called (or this object is destroyed),
    // cancelling any outstanding Run().
    async::Promise<std::monostate> close_promise_;

    // Shared with async callbacks.
    std::shared_ptr<State> state_;
};

}  // namespace launchdarkly::server_side::data_systems
