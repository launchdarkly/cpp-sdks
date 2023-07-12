#include <chrono>
#include <iomanip>
#include <optional>
#include <ostream>

#include <launchdarkly/data_sources/data_source_status_error_info.hpp>

namespace launchdarkly::common::data_sources {

std::ostream& operator<<(std::ostream& out,
                         DataSourceStatusErrorInfo const& error) {
    std::time_t as_time_t = std::chrono::system_clock::to_time_t(error.Time());
    out << "Error(" << error.Kind() << ", " << error.Message()
        << ", StatusCode(" << error.StatusCode() << "), Since("
        << std::put_time(std::gmtime(&as_time_t), "%Y-%m-%d %H:%M:%S") << "))";
    return out;
}

}  // namespace launchdarkly::common::data_sources
