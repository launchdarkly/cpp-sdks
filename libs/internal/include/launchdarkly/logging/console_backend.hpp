#pragma once

#include <launchdarkly/logging/log_backend.hpp>
#include <mutex>

namespace launchdarkly::logging {
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

    bool Enabled(LogLevel level) override;

    void Write(LogLevel level, std::string message) override;

   private:
    LogLevel level_;
    std::string name_;
    std::mutex write_mutex_;
};
}  // namespace launchdarkly::logging
