#include <iomanip>

#include <launchdarkly/client_side/data_source_status.hpp>

namespace launchdarkly::client_side::data_sources {

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

DataSourceStatus::DataSourceStatus(DataSourceStatus::DataSourceState state,
                                   DataSourceStatus::DateTime state_since,
                                   std::optional<ErrorInfo> last_error)
    : DataSourceStatusBase(state, state_since, std::move(last_error)) {}

DataSourceStatus::DataSourceStatus(DataSourceStatus const& item)
    : DataSourceStatusBase(item) {}

DataSourceStatus::DataSourceStatus(DataSourceStatus&& item)
    : DataSourceStatusBase(std::move(item)) {}
}  // namespace launchdarkly::client_side::data_sources
