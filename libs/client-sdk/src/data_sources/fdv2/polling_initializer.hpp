#pragma once

#include "fdv2_request_config.hpp"
#include "ifdv2_initializer.hpp"

#include <launchdarkly/async/promise.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/network/requester.hpp>

#include <boost/asio/any_io_executor.hpp>

#include <memory>
#include <string>

namespace launchdarkly::client_side::data_sources {

/**
 * Loads a basis with a single request to the FDv2 client polling endpoint.
 *
 * Threading model:
 *   Run() should only be called once at a time.
 *   Close() may be called concurrently with Run().
 *   This object may be safely destroyed once no call to Run() or Close() is
 *   in progress.
 */
class FDv2PollingInitializer final : public IFDv2Initializer {
   public:
    FDv2PollingInitializer(boost::asio::any_io_executor const& executor,
                           Logger const& logger,
                           FDv2RequestConfig const& request_config);

    ~FDv2PollingInitializer() override;

    async::Future<FDv2SourceResult> Run() override;

    void Close() override;

    [[nodiscard]] std::string const& Identity() const override;

   private:
    /** Interprets an HTTP response as a source result. */
    static FDv2SourceResult HandlePollResult(Logger const& logger,
                                             network::HttpResult const& res);

    // Logger is itself thread-safe and cheap to copy.
    Logger const logger_;

    // Immutable state.
    network::HttpRequest const request_;
    network::Requester const requester_;

    // Resolved when Close() is called, or when this object is destroyed,
    // cancelling any outstanding Run().
    async::Promise<std::monostate> close_promise_;
};

}  // namespace launchdarkly::client_side::data_sources
