#pragma once
#include <launchdarkly/connection.hpp>
#include <launchdarkly/data_sources/data_source_status_manager_base.hpp>
#include <launchdarkly/server_side/data_source_status.hpp>

#include <boost/signals2.hpp>

#include <functional>
#include <mutex>

namespace launchdarkly::server_side::data_components {

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

}  // namespace launchdarkly::server_side::data_components
