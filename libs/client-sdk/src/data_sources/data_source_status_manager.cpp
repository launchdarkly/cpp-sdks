#include <memory>
#include <mutex>
#include <sstream>
#include <utility>

#include <launchdarkly/connection.hpp>

#include "../boost_signal_connection.hpp"
#include "data_source_status_manager.hpp"

namespace launchdarkly::client_side::data_sources {

void DataSourceStatusManager::SetState(
    DataSourceStatus::DataSourceState state) {
    bool changed = UpdateState(state);
    if (changed) {
        data_source_status_signal_(std::move(Status()));
    }
}

void DataSourceStatusManager::SetState(
    DataSourceStatus::DataSourceState state,
    DataSourceStatus::ErrorInfo::StatusCodeType code,
    std::string message) {
    {
        std::lock_guard lock(status_mutex_);

        UpdateState(state);

        last_error_ = DataSourceStatus::ErrorInfo(
            DataSourceStatus::ErrorInfo::ErrorKind::kErrorResponse, code,
            message, std::chrono::system_clock::now());
    }

    data_source_status_signal_(std::move(Status()));
}
bool DataSourceStatusManager::UpdateState(
    DataSourceStatus::DataSourceState const& requested_state) {
    std::lock_guard lock(status_mutex_);

    // If initializing, then interruptions remain initializing.
    auto new_state =
        (requested_state == DataSourceStatus::DataSourceState::kInterrupted &&
         state_ == DataSourceStatus::DataSourceState::kInitializing)
            ? DataSourceStatus::DataSourceState::
                  kInitializing  // see comment on
                                 // IDataSourceUpdateSink.UpdateStatus
            : requested_state;
    auto changed = state_ != new_state;
    if (changed) {
        state_ = new_state;
        state_since_ = std::chrono::system_clock::now();
    }
    return changed;
}

void DataSourceStatusManager::SetState(
    DataSourceStatus::DataSourceState state,
    DataSourceStatus::ErrorInfo::ErrorKind kind,
    std::string message) {
    {
        std::lock_guard lock(status_mutex_);

        UpdateState(state);

        last_error_ = DataSourceStatus::ErrorInfo(
            kind, 0, std::move(message), std::chrono::system_clock::now());
    }

    data_source_status_signal_(Status());
}

void DataSourceStatusManager::SetError(
    DataSourceStatus::ErrorInfo::ErrorKind kind,
    std::string message) {
    {
        std::lock_guard lock(status_mutex_);
        last_error_ = DataSourceStatus::ErrorInfo(
            kind, 0, std::move(message), std::chrono::system_clock::now());
        state_since_ = std::chrono::system_clock::now();
    }

    data_source_status_signal_(Status());
}

void DataSourceStatusManager::SetError(
    DataSourceStatus::ErrorInfo::StatusCodeType code,
    std::string message) {
    {
        std::lock_guard lock(status_mutex_);
        last_error_ = DataSourceStatus::ErrorInfo(
            DataSourceStatus::ErrorInfo::ErrorKind::kErrorResponse, code,
            message, std::chrono::system_clock::now());
        state_since_ = std::chrono::system_clock::now();
    }
    data_source_status_signal_(Status());
}

DataSourceStatus DataSourceStatusManager::Status() {
    std::lock_guard lock(status_mutex_);
    return {state_, state_since_, last_error_};
}

std::unique_ptr<IConnection> DataSourceStatusManager::OnDataSourceStatusChange(
    std::function<void(data_sources::DataSourceStatus)> handler) {
    std::lock_guard lock{status_mutex_};
    return std::make_unique<
        ::launchdarkly::client_side::SignalConnection>(
        data_source_status_signal_.connect(handler));
}

DataSourceStatusManager::DataSourceStatusManager()
    : state_(DataSourceStatus::DataSourceState::kInitializing),
      state_since_(std::chrono::system_clock::now()) {}

}  // namespace launchdarkly::client_side::data_sources
