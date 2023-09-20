#pragma once

#include <launchdarkly/config/shared/built/data_source_config.hpp>
#include <launchdarkly/config/shared/built/http_properties.hpp>
#include <launchdarkly/config/shared/built/service_endpoints.hpp>
#include <launchdarkly/logging/logger.hpp>

#include "../data_source_event_handler.hpp"
#include "../data_source_interface.hpp"
#include "../data_source_status_manager.hpp"
#include "../data_source_update_sink.hpp"

#include "../memory_store/memory_store.hpp"

#include <boost/asio/any_io_executor.hpp>

namespace launchdarkly::server_side::data_sources {

class PullModeSource : public IDataSource {
   public:
    PullModeSource(config::shared::built::ServiceEndpoints const& endpoints,
                   config::shared::built::DataSourceConfig<
                       config::shared::ServerSDK> const& data_source_config,
                   config::shared::built::HttpProperties http_properties,
                   boost::asio::any_io_executor ioc,
                   DataSourceStatusManager& status_manager,
                   Logger const& logger);

    PullModeSource(PullModeSource const& item) = delete;
    PullModeSource(PullModeSource&& item) = delete;
    PullModeSource& operator=(PullModeSource const&) = delete;
    PullModeSource& operator=(PullModeSource&&) = delete;

    std::string Identity() const override;

    std::weak_ptr<ISynchronizer> GetSynchronizer() const override;
    std::weak_ptr<IBootstrapper> GetBootstrapper() const override;

    std::shared_ptr<FlagDescriptor> GetFlag(
        std::string const& key) const override;
    std::shared_ptr<SegmentDescriptor> GetSegment(
        std::string const& key) const override;
    std::unordered_map<std::string, std::shared_ptr<FlagDescriptor>> AllFlags()
        const override;
    std::unordered_map<std::string, std::shared_ptr<SegmentDescriptor>>
    AllSegments() const override;

   private:
    MemoryStore store_;
    std::shared_ptr<ISynchronizer> synchronizer_;
    std::shared_ptr<IBootstrapper> bootstrapper_;
};
}  // namespace launchdarkly::server_side::data_sources
