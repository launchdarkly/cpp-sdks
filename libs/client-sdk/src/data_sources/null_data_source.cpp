#include "null_data_source.hpp"

#include <boost/asio/post.hpp>

namespace launchdarkly::client_side::data_sources {

void NullDataSource::Start() {
    status_manager_.SetState(DataSourceStatus::DataSourceState::kSetOffline);
}

void NullDataSource::ShutdownAsync(std::function<void()> complete) {
    boost::asio::post(exec_, complete);
}

NullDataSource::NullDataSource(boost::asio::any_io_executor exec,
                               DataSourceStatusManager& status_manager)
    : status_manager_(status_manager), exec_(exec) {}

}  // namespace launchdarkly::client_side::data_sources
