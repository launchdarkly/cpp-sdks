#pragma once

#include "launchdarkly/logging/log_backend.hpp"
#include "logger.hpp"

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
 * Back-end to use when logging is disabled.
 */
class NullLoggerBackend : public launchdarkly::ILogBackend {
   public:
    /**
     * Always returns false.
     */
    bool Enabled(LogLevel level) override;

    /**
     * No-op.
     */
    void Write(LogLevel level, std::string message) override;
};
}  // namespace launchdarkly::logging
