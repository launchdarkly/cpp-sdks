#pragma once

#include "fdv2_request_config.hpp"
#include "ifdv2_initializer_factory.hpp"
#include "ifdv2_synchronizer_factory.hpp"

#include <launchdarkly/logging/logger.hpp>

#include <boost/asio/any_io_executor.hpp>

#include <chrono>
#include <memory>
#include <optional>

namespace launchdarkly::client_side::data_sources {

/**
 * Builds fresh FDv2PollingInitializer instances on demand.
 *
 * Thread-safe: Build() may be called from any thread, and the
 * configuration it hands to each source is fixed at construction.
 */
class FDv2PollingInitializerFactory final : public IFDv2InitializerFactory {
   public:
    FDv2PollingInitializerFactory(boost::asio::any_io_executor executor,
                                  Logger logger,
                                  FDv2RequestConfig request_config);

    std::unique_ptr<IFDv2Initializer> Build() override;

   private:
    boost::asio::any_io_executor const executor_;
    Logger const logger_;
    FDv2RequestConfig const request_config_;
};

/**
 * Builds fresh FDv2PollingSynchronizer instances on demand.
 *
 * Thread-safe: Build() may be called from any thread, and the
 * configuration it hands to each source is fixed at construction.
 */
class FDv2PollingSynchronizerFactory final : public IFDv2SynchronizerFactory {
   public:
    /**
     * @param last_poll When this context was last polled, if that is known,
     * so that the first poll after the synchronizer starts still respects the
     * interval.
     */
    FDv2PollingSynchronizerFactory(
        boost::asio::any_io_executor executor,
        Logger logger,
        FDv2RequestConfig request_config,
        std::chrono::seconds poll_interval,
        std::optional<std::chrono::steady_clock::time_point> last_poll);

    std::unique_ptr<IFDv2Synchronizer> Build() override;

   private:
    boost::asio::any_io_executor const executor_;
    Logger const logger_;
    FDv2RequestConfig const request_config_;
    std::chrono::seconds const poll_interval_;
    std::optional<std::chrono::steady_clock::time_point> const last_poll_;
};

/**
 * Builds fresh FDv2StreamingSynchronizer instances on demand.
 *
 * Thread-safe: Build() may be called from any thread, and the
 * configuration it hands to each source is fixed at construction.
 */
class FDv2StreamingSynchronizerFactory final : public IFDv2SynchronizerFactory {
   public:
    /**
     * @param poll_config Where to poll in answer to a `ping` event on the
     * stream. Must describe the same context as stream_config.
     */
    FDv2StreamingSynchronizerFactory(
        boost::asio::any_io_executor executor,
        Logger logger,
        FDv2RequestConfig stream_config,
        FDv2RequestConfig poll_config,
        std::chrono::milliseconds initial_reconnect_delay);

    std::unique_ptr<IFDv2Synchronizer> Build() override;

   private:
    boost::asio::any_io_executor const executor_;
    Logger const logger_;
    FDv2RequestConfig const stream_config_;
    FDv2RequestConfig const poll_config_;
    std::chrono::milliseconds const initial_reconnect_delay_;
};

}  // namespace launchdarkly::client_side::data_sources
