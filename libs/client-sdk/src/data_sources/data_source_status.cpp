#include <iomanip>

#include <launchdarkly/client_side/data_source_status.hpp>

namespace launchdarkly::client_side::data_sources {

char const* GetDataSourceStateName(DataSourceState state,
                                   char const* default_if_unknown) {
    switch (state) {
        case DataSourceStatus::DataSourceState::kInitializing:
            return "INITIALIZING";
        case DataSourceStatus::DataSourceState::kValid:
            return "VALID";
        case DataSourceStatus::DataSourceState::kInterrupted:
            return "INTERRUPTED";
        case DataSourceStatus::DataSourceState::kSetOffline:
            return "OFFLINE";
        case DataSourceStatus::DataSourceState::kShutdown:
            return "SHUTDOWN";
        default:
            return default_if_unknown;
    }
}

std::ostream& operator<<(std::ostream& out,
                         DataSourceStatus::DataSourceState const& state) {
    out << GetDataSourceStateName(state, "UNKNOWN");
    return out;
}

std::ostream& operator<<(std::ostream& out, DataSourceStatus const& status) {
    std::time_t as_time_t =
        std::chrono::system_clock::to_time_t(status.StateSince());
    out << "Status(" << status.State() << ", Since("
        << std::put_time(std::gmtime(&as_time_t), "%Y-%m-%d %H:%M:%S") << ")";
    if (status.LastError()) {
        out << ", " << status.LastError().value();
    }
    out << ")";
    return out;
}

}  // namespace launchdarkly::client_side::data_sources
