#include <memory>
#include <mutex>

#include "launchdarkly/client_side/connection.hpp"
#include "launchdarkly/client_side/data_sources/detail/data_source_status_manager.hpp"
#include "launchdarkly/client_side/detail/boost_signal_connection.hpp"

namespace launchdarkly::client_side::data_sources::detail {

void DataSourceStatusManager::SetState(DataSourceState state) {
    bool changed;
    {
        std::lock_guard lock(status_mutex_);
        changed = state_ != state;
        state_ = state;
        if (changed) {
            state_since_ = std::chrono::system_clock::now();
        }
    }
    if (changed) {
        data_source_status_signal_(std::move(Status()));
    }
}

void DataSourceStatusManager::SetError(ErrorInfo::ErrorKind kind,
                                      std::string message) {
    {
        std::lock_guard lock(status_mutex_);
        last_error_ =
            ErrorInfo(kind, 0, message, std::chrono::system_clock::now());
        state_since_ = std::chrono::system_clock::now();
    }

    data_source_status_signal_(Status());
}

void DataSourceStatusManager::SetError(ErrorInfo::StatusCodeType code) {
    // TODO: String message.
    {
        std::lock_guard lock(status_mutex_);
        last_error_ = ErrorInfo(ErrorInfo::ErrorKind::kErrorResponse, code, "",
                                std::chrono::system_clock::now());
        state_since_ = std::chrono::system_clock::now();
    }
    data_source_status_signal_(Status());
}

DataSourceStatus DataSourceStatusManager::Status() {
    std::lock_guard lock(status_mutex_);
    return DataSourceStatus(state_, state_since_, last_error_);
}

std::unique_ptr<IConnection> DataSourceStatusManager::OnDataSourceStatusChange(
    std::function<void(data_sources::DataSourceStatus)> handler) {
    std::lock_guard lock{status_mutex_};
    return std::make_unique<
        ::launchdarkly::client_side::detail::SignalConnection>(
        data_source_status_signal_.connect(handler));
}

DataSourceStatusManager::DataSourceStatusManager()
    : state_(DataSourceState::kInitializing),
      state_since_(std::chrono::system_clock::now()) {}

}  // namespace launchdarkly::client_side::data_sources::detail
