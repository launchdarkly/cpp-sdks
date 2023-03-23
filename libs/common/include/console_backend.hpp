#pragma once

#include "log_backend.hpp"

namespace launchdarkly {
/**
 * Basic console back-end. Writes `kError` level to stderr and others to stdout.
 */
class ConsoleBackend : public ILogBackend {
   public:
    ConsoleBackend(LogLevel level, std::string name);

    bool enabled(LogLevel level) override;

    void write(LogLevel level, std::string message) override;

   private:
    LogLevel level_;
    std::string name_;
};
}  // namespace launchdarkly
