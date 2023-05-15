#include "null_logger.hpp"

namespace launchdarkly::logging {

bool NullLoggerBackend::Enabled(LogLevel level) {
    return false;
}

void NullLoggerBackend::Write(LogLevel level, std::string message) {}

Logger NullLogger() {
    return {std::make_unique<NullLoggerBackend>()};
}

}  // namespace launchdarkly::logging
