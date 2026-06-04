#pragma once

#include "../../data_components/status_notifications/data_source_status_manager.hpp"
#include "../../data_interfaces/source/ifdv2_synchronizer_factory.hpp"

#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/server_side/config/built/all_built.hpp>
#include <launchdarkly/server_side/config/built/data_system/fdv2_config.hpp>

#include <boost/asio/any_io_executor.hpp>

namespace launchdarkly::server_side::data_systems {

/**
 * Builds fresh FDv2StreamingSynchronizer instances on demand.
 */
class FDv2StreamingSynchronizerFactory final
    : public data_interfaces::IFDv2SynchronizerFactory {
   public:
    FDv2StreamingSynchronizerFactory(
        boost::asio::any_io_executor executor,
        Logger logger,
        config::built::ServiceEndpoints endpoints,
        config::built::HttpProperties http_properties,
        config::built::FDv2Config::StreamingConfig streaming);

    std::unique_ptr<data_interfaces::IFDv2Synchronizer> Build() override;

   private:
    boost::asio::any_io_executor const executor_;
    Logger const logger_;
    std::string const streaming_base_url_;
    config::built::HttpProperties const http_properties_;
    config::built::FDv2Config::StreamingConfig const streaming_;
};

/**
 * Builds fresh FDv2PollingSynchronizer instances on demand.
 */
class FDv2PollingSynchronizerFactory final
    : public data_interfaces::IFDv2SynchronizerFactory {
   public:
    FDv2PollingSynchronizerFactory(
        boost::asio::any_io_executor executor,
        Logger logger,
        config::built::ServiceEndpoints endpoints,
        config::built::HttpProperties http_properties,
        config::built::FDv2Config::PollingConfig polling);

    std::unique_ptr<data_interfaces::IFDv2Synchronizer> Build() override;

   private:
    boost::asio::any_io_executor const executor_;
    Logger const logger_;
    std::string const polling_base_url_;
    config::built::HttpProperties const http_properties_;
    config::built::FDv2Config::PollingConfig const polling_;
};

/**
 * Builds fresh FDv1AdapterSynchronizer instances wrapping a freshly-built
 * FDv1 StreamingDataSource. Reports IsFDv1Fallback() = true.
 */
class FDv1StreamingAdapterFactory final
    : public data_interfaces::IFDv2SynchronizerFactory {
   public:
    FDv1StreamingAdapterFactory(
        boost::asio::any_io_executor executor,
        Logger logger,
        data_components::DataSourceStatusManager* status_manager,
        config::built::ServiceEndpoints endpoints,
        config::built::FDv2Config::FDv1StreamingConfig streaming,
        config::built::HttpProperties http_properties);

    std::unique_ptr<data_interfaces::IFDv2Synchronizer> Build() override;

    [[nodiscard]] bool IsFDv1Fallback() const override { return true; }

   private:
    boost::asio::any_io_executor const executor_;
    Logger const logger_;
    // Non-owning. Provided by the orchestrator; must outlive this factory.
    data_components::DataSourceStatusManager* const status_manager_;
    config::built::ServiceEndpoints const endpoints_;
    config::built::FDv2Config::FDv1StreamingConfig const streaming_;
    config::built::HttpProperties const http_properties_;
};

/**
 * Builds fresh FDv1AdapterSynchronizer instances wrapping a freshly-built
 * FDv1 PollingDataSource. Reports IsFDv1Fallback() = true.
 */
class FDv1PollingAdapterFactory final
    : public data_interfaces::IFDv2SynchronizerFactory {
   public:
    FDv1PollingAdapterFactory(
        boost::asio::any_io_executor executor,
        Logger logger,
        data_components::DataSourceStatusManager* status_manager,
        config::built::ServiceEndpoints endpoints,
        config::built::FDv2Config::FDv1PollingConfig polling,
        config::built::HttpProperties http_properties);

    std::unique_ptr<data_interfaces::IFDv2Synchronizer> Build() override;

    [[nodiscard]] bool IsFDv1Fallback() const override { return true; }

   private:
    boost::asio::any_io_executor const executor_;
    Logger const logger_;
    // Non-owning. Provided by the orchestrator; must outlive this factory.
    data_components::DataSourceStatusManager* const status_manager_;
    config::built::ServiceEndpoints const endpoints_;
    config::built::FDv2Config::FDv1PollingConfig const polling_;
    config::built::HttpProperties const http_properties_;
};

}  // namespace launchdarkly::server_side::data_systems
