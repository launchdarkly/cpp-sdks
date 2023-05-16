#pragma once

#include <launchdarkly/logging/log_backend.hpp>
#include <launchdarkly/logging/logger.hpp>

namespace launchdarkly::logging {
/**
 * Creates a throwaway logger suitable for tests where assertions about the
 * log messages aren't relevant.
 *
 * @return Logger suitable for passing to functions accepting a
 * launchdarkly::Logger.
 */
launchdarkly::Logger NullLogger();

/**
 * Backend to use when logging is disabled.
 */
class NullLoggerBackend : public launchdarkly::ILogBackend {
   public:
    /**
     * Always returns false.
     */
    bool Enabled(LogLevel level) noexcept override;

    /**
     * No-op.
     */
    void Write(LogLevel level, std::string message) noexcept override;
};
}  // namespace launchdarkly::logging
