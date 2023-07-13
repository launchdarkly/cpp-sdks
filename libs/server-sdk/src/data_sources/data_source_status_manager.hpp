#pragma once

#include <functional>
#include <mutex>

#include <boost/signals2.hpp>

#include <launchdarkly/connection.hpp>
#include <launchdarkly/data_sources/data_source_status_manager.hpp>
#include <launchdarkly/server_side/data_source_status.hpp>

namespace launchdarkly::server_side::data_sources {

class DataSourceStatusManager
    : public internal::data_sources::DataSourceStatusManagerBase<
          DataSourceStatus,
          IDataSourceStatusProvider> {
   public:
    DataSourceStatusManager() = default;

    ~DataSourceStatusManager() override = default;
    DataSourceStatusManager(DataSourceStatusManager const& item) = delete;
    DataSourceStatusManager(DataSourceStatusManager&& item) = delete;
    DataSourceStatusManager& operator=(DataSourceStatusManager const&) = delete;
    DataSourceStatusManager& operator=(DataSourceStatusManager&&) = delete;
};

}  // namespace launchdarkly::server_side::data_sources
