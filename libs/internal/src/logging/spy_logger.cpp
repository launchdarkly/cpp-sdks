#include <launchdarkly/logging/spy_logger.hpp>

namespace launchdarkly::logging {

SpyLoggerBackend::SpyLoggerBackend(std::vector<std::string>& messages)
    : messages_(messages) {}

bool SpyLoggerBackend::Enabled(LogLevel level) noexcept {
    return true;
}

void SpyLoggerBackend::Write(LogLevel level, std::string message) noexcept {
    messages_.push_back(std::move(message));
}

Logger SpyLogger(std::vector<std::string>& messages) {
    return {std::make_unique<SpyLoggerBackend>(messages)};
}

}  // namespace launchdarkly::logging
