#pragma once

#include "../../data_interfaces/source/ifdv2_initializer.hpp"

#include <launchdarkly/data_model/selector.hpp>
#include <launchdarkly/fdv2_protocol_handler.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/network/asio_requester.hpp>
#include <launchdarkly/server_side/config/built/all_built.hpp>

#include <boost/asio/any_io_executor.hpp>

#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>

namespace launchdarkly::server_side::data_systems {

/**
 * FDv2 polling initializer. Makes a single HTTP GET to the FDv2 polling
 * endpoint, parses the response via the FDv2 protocol state machine, and
 * returns the result. Implements IFDv2Initializer (blocking, one-shot).
 */
class FDv2PollingInitializer final
    : public data_interfaces::IFDv2Initializer {
   public:
    FDv2PollingInitializer(
        boost::asio::any_io_executor const& executor,
        Logger const& logger,
        config::built::ServiceEndpoints const& endpoints,
        config::built::HttpProperties const& http_properties,
        data_model::Selector selector,
        std::optional<std::string> filter_key);

    data_interfaces::FDv2SourceResult Run() override;

    void Close() override;

    [[nodiscard]] std::string const& Identity() const override;

   private:
    data_interfaces::FDv2SourceResult HandlePollResult(
        network::HttpResult const& res);

    Logger const& logger_;
    network::AsioRequester requester_;
    network::HttpRequest request_;
    FDv2ProtocolHandler protocol_handler_;

    std::mutex mutex_;
    std::condition_variable cv_;
    bool closed_ = false;  // guarded by mutex_
};

}  // namespace launchdarkly::server_side::data_systems
