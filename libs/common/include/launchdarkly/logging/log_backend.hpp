#pragma once

#include <memory>
#include <optional>
#include <string>

#include "log_level.hpp"

namespace launchdarkly {
/**
 * Interface for logging back-ends.
 *
 * @example ../src/ConsoleBackend.hpp
 */
class ILogBackend {
   public:
    /**
     * Check if the specified log level is enabled.
     * @param level The log level to check.
     * @return Returns true if the level is enabled.
     */
    virtual bool Enabled(LogLevel level) = 0;

    /**
     * Write a message to the specified level.
     * @param level The level to Write the message for.
     * @param message The message to Write.
     */
    virtual void Write(LogLevel level, std::string message) = 0;

    virtual ~ILogBackend(){};
};
}  // namespace launchdarkly