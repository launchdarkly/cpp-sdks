#include "launchdarkly/client_side/data_sources/detail/data_source_status_manager.hpp"

namespace launchdarkly::client_side::data_sources::detail {

void DataSourceStateManager::SetState(DataSourceState state) {
    auto changed = state_ != state;
    state_ = state;
    if (changed) {
        handler_(std::move(Status()));
    }
}

void DataSourceStateManager::SetError(std::optional<ErrorInfo> error) {
    last_error_ = error;
    handler_(Status());
}

DataSourceStatus DataSourceStateManager::Status() {
    return DataSourceStatus(state_, state_since_, last_error_);
}

DataSourceStateManager::DataSourceStateManager(
    DataSourceStateManager::DataSourceStatusHandler handler)
    : handler_(handler),
      state_(DataSourceState::kInitializing),
      state_since_(std::chrono::system_clock::now()) {}

}  // namespace launchdarkly::client_side::data_sources::detail
