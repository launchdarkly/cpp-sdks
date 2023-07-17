#pragma once

#include <launchdarkly/logging/log_backend.hpp>
#include <launchdarkly/logging/logger.hpp>

#include <vector>

namespace launchdarkly::logging {
/**
 * Creates a logger for tests where log messages need to be inspected.
 *
 * @return Logger suitable for passing to functions accepting a
 * launchdarkly::Logger.
 */
launchdarkly::Logger SpyLogger(std::vector<std::string>& messages);

class SpyLoggerBackend : public launchdarkly::ILogBackend {
   public:
    SpyLoggerBackend(std::vector<std::string>& messages);

    /**
     * Always returns true.
     */
    bool Enabled(LogLevel level) noexcept override;

    /**
     * Records the message internally.
     */
    void Write(LogLevel level, std::string message) noexcept override;

   private:
    std::vector<std::string>& messages_;
};
}  // namespace launchdarkly::logging
