#pragma once

#include "../../../data_interfaces/destination/idestination.hpp"
#include "../../../data_interfaces/source/ipush_source.hpp"

#include "../../status_notifications/data_source_status_manager.hpp"

#include <boost/asio/any_io_executor.hpp>

namespace launchdarkly::server_side::data_components {

class NullDataSource : public data_interfaces::IPushSource {
   public:
    explicit NullDataSource(boost::asio::any_io_executor exec,
                            DataSourceStatusManager& status_manager);

    void Init(std::optional<data_model::SDKDataSet> initial_data,
              data_interfaces::IDestination& destination) override;
    void Start() override;
    void ShutdownAsync(std::function<void()>) override;

    [[nodiscard]] std::string const& Identity() const override;

   private:
    DataSourceStatusManager& status_manager_;
    boost::asio::any_io_executor exec_;
};

}  // namespace launchdarkly::server_side::data_components
