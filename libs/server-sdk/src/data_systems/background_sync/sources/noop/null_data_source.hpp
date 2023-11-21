#pragma once

#include "../../../../data_components/status_notifications/data_source_status_manager.hpp"
#include "../../../../data_interfaces/source/idata_synchronizer.hpp"

#include <boost/asio/any_io_executor.hpp>

namespace launchdarkly::server_side::data_systems {

class NullDataSource : public data_interfaces::IDataSynchronizer {
   public:
    explicit NullDataSource(
        boost::asio::any_io_executor exec,
        data_components::DataSourceStatusManager& status_manager);

    void StartAsync(data_interfaces::IDestination* destination,
                    data_model::SDKDataSet const* initial_data) override;
    void ShutdownAsync(std::function<void()>) override;

    [[nodiscard]] std::string const& Identity() const override;

   private:
    data_components::DataSourceStatusManager& status_manager_;
    boost::asio::any_io_executor exec_;
};

}  // namespace launchdarkly::server_side::data_systems
