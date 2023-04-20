
#include <utility>

#include "launchdarkly/client_side/data_sources/data_source_status.hpp"
namespace launchdarkly::client_side::data_sources {

DataSourceStatus::ErrorInfo::ErrorKind DataSourceStatus::ErrorInfo::Kind()
    const {
    return kind_;
}
DataSourceStatus::ErrorInfo::StatusCodeType
DataSourceStatus::ErrorInfo::StatusCode() const {
    return status_code_;
}
std::string const& DataSourceStatus::ErrorInfo::Message() const {
    return message_;
}
DataSourceStatus::DateTime DataSourceStatus::ErrorInfo::Time() const {
    return time_;
}

DataSourceStatus::ErrorInfo::ErrorInfo(ErrorInfo::ErrorKind kind,
                                       ErrorInfo::StatusCodeType status_code,
                                       std::string message,
                                       DataSourceStatus::DateTime time)
    : kind_(kind),
      status_code_(status_code),
      message_(std::move(message)),
      time_(time) {}

DataSourceStatus::DataSourceState DataSourceStatus::State() const {
    return state_;
}

DataSourceStatus::DateTime DataSourceStatus::StateSince() const {
    return state_since_;
}

std::optional<DataSourceStatus::ErrorInfo> DataSourceStatus::LastError() const {
    return last_error_;
}

DataSourceStatus::DataSourceStatus(DataSourceState state,
                                   DataSourceStatus::DateTime state_since,
                                   std::optional<ErrorInfo> last_error)
    : state_(state),
      state_since_(state_since),
      last_error_(std::move(last_error)) {}

std::ostream& operator<<(std::ostream& out,
                         DataSourceStatus::DataSourceState const& state) {
    switch (state) {
        case DataSourceStatus::DataSourceState::kInitializing:
            out << "INITIALIZING";
            break;
        case DataSourceStatus::DataSourceState::kValid:
            out << "VALID";
            break;
        case DataSourceStatus::DataSourceState::kInterrupted:
            out << "INTERRUPTED";
            break;
        case DataSourceStatus::DataSourceState::kSetOffline:
            out << "OFFLINE";
            break;
        case DataSourceStatus::DataSourceState::kShutdown:
            out << "SHUTDOWN";
            break;
    }

    return out;
}

std::ostream& operator<<(std::ostream& out,
                         DataSourceStatus::ErrorInfo::ErrorKind const& kind) {
    switch (kind) {
        case DataSourceStatus::ErrorInfo::ErrorKind::kUnknown:
            out << "UNKNOWN";
            break;
        case DataSourceStatus::ErrorInfo::ErrorKind::kNetworkError:
            out << "NETWORK_ERROR";
            break;
        case DataSourceStatus::ErrorInfo::ErrorKind::kErrorResponse:
            out << "ERROR_RESPONSE";
            break;
        case DataSourceStatus::ErrorInfo::ErrorKind::kInvalidData:
            out << "INVALID_DATA";
            break;
        case DataSourceStatus::ErrorInfo::ErrorKind::kStoreError:
            out << "STORE_ERROR";
            break;
    }
    return out;
}

}  // namespace launchdarkly::client_side::data_sources
