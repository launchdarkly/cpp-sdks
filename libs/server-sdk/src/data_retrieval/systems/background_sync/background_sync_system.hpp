#pragma once

#include "../../interfaces/data_system.hpp"
#include "../../memory_store/memory_store.hpp"
#include "../../status_notifications/data_source_status_manager.hpp"

#include <launchdarkly/config/shared/built/data_source_config.hpp>
#include <launchdarkly/config/shared/built/http_properties.hpp>
#include <launchdarkly/config/shared/built/service_endpoints.hpp>
#include <launchdarkly/data_model/descriptors.hpp>
#include <launchdarkly/logging/logger.hpp>

#include <boost/asio/any_io_executor.hpp>

namespace launchdarkly::server_side::data_retrieval {

class BackgroundSync : public IDataSystem {
   public:
    BackgroundSync(config::shared::built::ServiceEndpoints const& endpoints,
                   config::shared::built::DataSourceConfig<
                       config::shared::ServerSDK> const& data_source_config,
                   config::shared::built::HttpProperties http_properties,
                   boost::asio::any_io_executor ioc,
                   DataSourceStatusManager& status_manager,
                   Logger const& logger);

    BackgroundSync(BackgroundSync const& item) = delete;
    BackgroundSync(BackgroundSync&& item) = delete;
    BackgroundSync& operator=(BackgroundSync const&) = delete;
    BackgroundSync& operator=(BackgroundSync&&) = delete;

    std::string const& Identity() const override;

    void Initialize() override;

    std::shared_ptr<data_model::FlagDescriptor> GetFlag(
        std::string const& key) const override;
    std::shared_ptr<data_model::SegmentDescriptor> GetSegment(
        std::string const& key) const override;
    std::unordered_map<std::string, std::shared_ptr<data_model::FlagDescriptor>>
    AllFlags() const override;
    std::unordered_map<std::string,
                       std::shared_ptr<data_model::SegmentDescriptor>>
    AllSegments() const override;

   private:
    MemoryStore store_;
};
}  // namespace launchdarkly::server_side::data_retrieval
