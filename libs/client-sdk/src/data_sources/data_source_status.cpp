
#include "launchdarkly/client_side/data_sources/data_source_status.hpp"
namespace launchdarkly::client_side::data_sources {

ErrorInfo::ErrorKind ErrorInfo::Kind() const {
    return kind_;
}
ErrorInfo::StatusCodeType ErrorInfo::StatusCode() const {
    return status_code_;
}
std::string const& ErrorInfo::Message() const {
    return message_;
}
ErrorInfo::DateTime ErrorInfo::Time() const {
    return time_;
}

ErrorInfo::ErrorInfo(ErrorInfo::ErrorKind kind,
                     ErrorInfo::StatusCodeType status_code,
                     std::string message,
                     ErrorInfo::DateTime time)
    : kind_(kind), status_code_(status_code), message_(message), time_(time) {}

DataSourceState DataSourceStatus::State() const {
    return state_;
}

DataSourceStatus::DateTime DataSourceStatus::StateSince() const {
    return state_since_;
}

std::optional<ErrorInfo> DataSourceStatus::LastError() const {
    return last_error_;
}

DataSourceStatus::DataSourceStatus(DataSourceState state,
                                   DataSourceStatus::DateTime state_since,
                                   std::optional<ErrorInfo> last_error)
    : state_(state), state_since_(state_since), last_error_(last_error) {}
}  // namespace launchdarkly::client_side::data_sources
