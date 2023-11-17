#pragma once

#include "../data_components/status_notifications/data_source_status_manager.hpp"

#include <launchdarkly/data_sources/data_source.hpp>

#include <boost/asio/any_io_executor.hpp>

namespace launchdarkly::server_side::data_sources {

class NullDataSource final : public ::launchdarkly::data_sources::IDataSource {
   public:
    explicit NullDataSource(
        boost::asio::any_io_executor exec,
        data_components::DataSourceStatusManager& status_manager);
    void Start() override;
    void ShutdownAsync(std::function<void()>) override;

   private:
    data_components::DataSourceStatusManager& status_manager_;
    boost::asio::any_io_executor exec_;
};

}  // namespace launchdarkly::server_side::data_sources
