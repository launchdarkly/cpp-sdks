#include <memory>
#include <mutex>
#include <sstream>

#include <launchdarkly/connection.hpp>
#include <launchdarkly/signals/boost_signal_connection.hpp>

#include "data_source_status_manager.hpp"

namespace launchdarkly::server_side::data_sources {

void DataSourceStatusManager::SetState(
    DataSourceStatus::DataSourceState state) {
    manager_.SetState(state);
}

void DataSourceStatusManager::SetState(
    DataSourceStatus::DataSourceState state,
    DataSourceStatus::ErrorInfo::StatusCodeType code,
    std::string message) {
    manager_.SetState(state, code, message);
}

void DataSourceStatusManager::SetState(
    DataSourceStatus::DataSourceState state,
    DataSourceStatus::ErrorInfo::ErrorKind kind,
    std::string message) {
    manager_.SetState(state, kind, message);
}

void DataSourceStatusManager::SetError(
    DataSourceStatus::ErrorInfo::ErrorKind kind,
    std::string message) {
    manager_.SetError(kind, message);
}

void DataSourceStatusManager::SetError(
    DataSourceStatus::ErrorInfo::StatusCodeType code,
    std::string message) {
    manager_.SetError(code, message);
}

DataSourceStatus DataSourceStatusManager::Status() const {
    return manager_.Status();
}

std::unique_ptr<IConnection> DataSourceStatusManager::OnDataSourceStatusChange(
    std::function<void(data_sources::DataSourceStatus)> handler) {
    return manager_.OnDataSourceStatusChange(handler);
}

std::unique_ptr<IConnection>
DataSourceStatusManager::OnDataSourceStatusChangeEx(
    std::function<bool(data_sources::DataSourceStatus)> handler) {
    return manager_.OnDataSourceStatusChangeEx(handler);
}
DataSourceStatusManager::DataSourceStatusManager() {}

}  // namespace launchdarkly::client_side::data_sources
