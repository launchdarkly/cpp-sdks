#include <iomanip>

#include <launchdarkly/server_side/data_source_status.hpp>

namespace launchdarkly::server_side::data_sources {

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
        case DataSourceStatus::DataSourceState::kOff:
            out << "OFF";
            break;
    }

    return out;
}

std::ostream& operator<<(std::ostream& out, DataSourceStatus const& status) {
    std::time_t as_time_t =
        std::chrono::system_clock::to_time_t(status.StateSince());
    out << "Status(" << status.State() << ", Since("
        << std::put_time(std::gmtime(&as_time_t), "%Y-%m-%d %H:%M:%S") << ")";
    if (status.LastError().has_value()) {
        out << ", " << status.LastError().value();
    }
    out << ")";
    return out;
}

}  // namespace launchdarkly::server_side::data_sources
