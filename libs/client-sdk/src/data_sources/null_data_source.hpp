#pragma once

#include "data_source.hpp"
#include "data_source_status_manager.hpp"

namespace launchdarkly::client_side::data_sources {

class NullDataSource : public IDataSource {
   public:
    explicit NullDataSource(DataSourceStatusManager& status_manager);
    void Start() override;
    void ShutdownAsync(std::function<void()>) override;

   private:
    DataSourceStatusManager& status_manager_;
};

}  // namespace launchdarkly::client_side::data_sources
