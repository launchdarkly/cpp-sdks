#pragma once

#include <memory>
#include <optional>
#include <string>

#include "log_level.hpp"

namespace launchdarkly {
/**
 * Interface for logging back-ends.
 *
 * For a reference implementation refer to console_backend.hpp/cpp.
 */
class ILogBackend {
   public:
    /**
     * Check if the specified log level is enabled.
     * @param level The log level to check.
     * @return Returns true if the level is enabled.
     */
    virtual bool Enabled(LogLevel level) noexcept = 0;

    /**
     * Write a message to the specified level. This method must be thread safe.
     * @param level The level to write the message to.
     * @param message The message to write.
     */
    virtual void Write(LogLevel level, std::string message) noexcept = 0;

    virtual ~ILogBackend() = default;
    ILogBackend(ILogBackend const& item) = delete;
    ILogBackend(ILogBackend&& item) = delete;
    ILogBackend& operator=(ILogBackend const&) = delete;
    ILogBackend& operator=(ILogBackend&&) = delete;

   protected:
    ILogBackend() = default;
};
}  // namespace launchdarkly
