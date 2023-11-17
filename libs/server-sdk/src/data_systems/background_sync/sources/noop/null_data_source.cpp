#include "null_data_source.hpp"

#include <boost/asio/post.hpp>

namespace launchdarkly::server_side::data_systems {

void NullDataSource::StartAsync() {
    status_manager_.SetState(DataSourceStatus::DataSourceState::kValid);
}

void NullDataSource::ShutdownAsync(std::function<void()> complete) {
    boost::asio::post(exec_, complete);
}

void NullDataSource::Init(std::optional<data_model::SDKDataSet> initial_data) {}

std::string const& NullDataSource::Identity() const {
    static std::string const identity = "no-op data source";
    return identity;
}

NullDataSource::NullDataSource(
    boost::asio::any_io_executor exec,
    data_components::DataSourceStatusManager& status_manager)
    : status_manager_(status_manager), exec_(exec) {}

}  // namespace launchdarkly::server_side::data_systems
