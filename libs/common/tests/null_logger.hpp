#pragma once
#include "launchdarkly/logging/logger.hpp"

/**
 * Creates a throwaway logger suitable for tests where assertions about the
 * log messages aren't relevant.
 *
 * @return Logger suitable for passing to functions accepting a
 * launchdarkly::Logger.
 */
launchdarkly::Logger NullLogger();
class NullLoggerBackend : public launchdarkly::ILogBackend {
   public:
    /**
     * Always returns false.
     */
    bool Enabled(launchdarkly::LogLevel level) override;

    /**
     * No-op.
     */
    void Write(launchdarkly::LogLevel level, std::string message) override;
};
