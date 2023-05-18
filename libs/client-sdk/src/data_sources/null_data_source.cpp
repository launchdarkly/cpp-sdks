#include "null_data_source.hpp"

namespace launchdarkly::client_side::data_sources {

void NullDataSource::Start() {
    status_manager_.SetState(DataSourceStatus::DataSourceState::kSetOffline);
}

void NullDataSource::ShutdownAsync(std::function<void()> complete) {
    complete();
}
NullDataSource::NullDataSource(DataSourceStatusManager& status_manager)
    : status_manager_(status_manager) {}

}  // namespace launchdarkly::client_side::data_sources
