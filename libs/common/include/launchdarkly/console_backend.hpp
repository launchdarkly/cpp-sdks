#pragma once

#include "log_backend.hpp"

namespace launchdarkly {
/**
 * Basic console back-end. Writes `kError` level to stderr and others to stdout.
 */
class ConsoleBackend : public ILogBackend {
   public:
    /**
     * Constructs a ConsoleBackend which logs at the given level or above.
     * @param level Emit logs at this level or above.
     * @param name Log tag.
     */
    ConsoleBackend(LogLevel level, std::string name);
    /**
     * Constructs a ConsoleBackend which logs at the level given by the
     * LD_LOG_LEVEL environment variable, or info level by default.
     * @param name Log tag.
     */
    ConsoleBackend(std::string name);

    bool enabled(LogLevel level) override;

    void write(LogLevel level, std::string message) override;

   private:
    LogLevel level_;
    std::string name_;
};
}  // namespace launchdarkly
