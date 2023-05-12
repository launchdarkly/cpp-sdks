#include "null_logger.hpp"

using launchdarkly::Logger;
using launchdarkly::LogLevel;

bool NullLoggerBackend::Enabled(LogLevel level) {
    return false;
}

void NullLoggerBackend::Write(LogLevel level, std::string message) {}

Logger NullLogger() {
    return {std::make_unique<NullLoggerBackend>()};
}
