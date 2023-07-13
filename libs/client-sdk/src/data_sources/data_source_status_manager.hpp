#pragma once

#include <functional>
#include <mutex>

#include <boost/signals2.hpp>

#include <launchdarkly/client_side/data_source_status.hpp>
#include <launchdarkly/connection.hpp>
#include <launchdarkly/data_sources/data_source_status_manager.hpp>

namespace launchdarkly::client_side::data_sources {

class DataSourceStatusManager
    : public internal::data_sources::DataSourceStatusManagerBase<
          DataSourceStatus,
          IDataSourceStatusProvider> {
   public:
    DataSourceStatusManager();

    ~DataSourceStatusManager() override = default;
    DataSourceStatusManager(DataSourceStatusManager const& item) = delete;
    DataSourceStatusManager(DataSourceStatusManager&& item) = delete;
    DataSourceStatusManager& operator=(DataSourceStatusManager const&) = delete;
    DataSourceStatusManager& operator=(DataSourceStatusManager&&) = delete;
};

}  // namespace launchdarkly::client_side::data_sources
