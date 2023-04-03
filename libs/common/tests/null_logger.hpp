#pragma once
#include "logger.hpp"

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
    bool enabled(launchdarkly::LogLevel level) override;

    /**
     * No-op.
     */
    void write(launchdarkly::LogLevel level, std::string message) override;
};
