#include <iostream>

#include <launchdarkly/console_backend.hpp>

namespace launchdarkly {
ConsoleBackend::ConsoleBackend(LogLevel level, std::string name)
    : level_(level), name_(std::move(name)) {
    // Update the name to be enclosed in brackets and have a space after.
    // "LaunchDarkly" -> "[LaunchDarkly] "
    name_.insert(0, 1, '[');
    name_.append("] ");
}

ConsoleBackend::ConsoleBackend(std::string name)
    : ConsoleBackend(
          GetLogLevelEnum(std::getenv("LD_LOG_LEVEL"), LogLevel::kInfo),
          std::move(name)) {}

bool ConsoleBackend::enabled(LogLevel level) {
    return level >= level_;
}

void ConsoleBackend::write(LogLevel level, std::string message) {
    if (enabled(level)) {
        if (level == LogLevel::kError) {
            std::cerr << name_ << message << std::endl;
        } else {
            std::cout << name_ << message << std::endl;
        }
    }
}
}  // namespace launchdarkly
