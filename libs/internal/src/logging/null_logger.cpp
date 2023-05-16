#include <launchdarkly/logging/null_logger.hpp>

namespace launchdarkly::logging {

bool NullLoggerBackend::Enabled(LogLevel level) noexcept {
    return false;
}

void NullLoggerBackend::Write(LogLevel level, std::string message) noexcept {}

Logger NullLogger() {
    return {std::make_unique<NullLoggerBackend>()};
}

}  // namespace launchdarkly::logging
