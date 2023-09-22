#pragma once

#include "../../data_source_status_manager.hpp"
#include "../../interfaces/data_destination.hpp"
#include "../../interfaces/data_push_source.hpp"

#include <boost/asio/any_io_executor.hpp>

namespace launchdarkly::server_side::data_retrieval {

class NullDataSource : public ISynchronizer {
   public:
    explicit NullDataSource(boost::asio::any_io_executor exec,
                            DataSourceStatusManager& status_manager);

    void Init(std::optional<data_model::SDKDataSet> initial_data,
              IDataDestination& destination) override;
    void Start() override;
    void ShutdownAsync(std::function<void()>) override;

   private:
    DataSourceStatusManager& status_manager_;
    boost::asio::any_io_executor exec_;
};

}  // namespace launchdarkly::server_side::data_retrieval
