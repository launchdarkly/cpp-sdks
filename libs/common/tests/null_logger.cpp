#include "null_logger.hpp"

using launchdarkly::Logger;
using launchdarkly::LogLevel;

bool NullLoggerBackend::enabled(LogLevel level) {
    return false;
}

void NullLoggerBackend::write(LogLevel level, std::string message) {}

Logger NullLogger() {
    return {std::make_unique<NullLoggerBackend>()};
}
