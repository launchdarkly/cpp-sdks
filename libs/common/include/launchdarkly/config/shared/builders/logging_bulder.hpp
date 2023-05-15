#pragma once

#include <launchdarkly/config/shared/built/logging.hpp>
#include <launchdarkly/logging/log_backend.hpp>

#include <variant>

namespace launchdarkly::config::shared::builders {

class LoggingBuilder {
   public:
    /**
     * Class for configuring built in logging using the SDKs console logger.
     */
    class BasicLogging {
       public:
        /**
         * Set the enabled log level.
         *
         * @return A reference to this builder.
         */
        BasicLogging& Level(LogLevel level);

        /**
         * Set a name for this logger. This name will be included at the start
         * of log entries in square brackets.
         *
         * If the name was "LaunchDarkly", then log entries will be prefixed
         * with "[LaunchDarkly]".
         *
         * @param name
         * @return
         */
        BasicLogging& Name(std::string name);

       private:
        LogLevel level_;
        std::string name_;
        friend class LoggingBuilder;
    };

    class CustomLogging {
       public:
        /**
         * Set the backend to use for logging. The provided back-end should
         * be thread-safe.
         * @param backend The implementation of the backend.
         * @return A reference to this builder.
         */
        CustomLogging& Backend(std::shared_ptr<ILogBackend> backend);

       private:
        std::shared_ptr<ILogBackend> backend_;
        friend class LoggingBuilder;
    };

    class NoLogging {};

    using LoggingType = std::variant<BasicLogging, CustomLogging, NoLogging>;

    /**
     * Set the type of logging to use.
     *
     * Disable logging:
     * ```
     * builder.Logging(LoggingBuilder::NoLogging())
     * ```
     *
     * Custom logging level:
     * ```
     * builder.Logging(LoggingBuilder::BasicLogging().Level(LogLevel::kDebug))
     * ```
     *
     * @param logging
     * @return
     */
    LoggingBuilder& Logging(LoggingType logging);

    built::Logging Build() const;

   private:
    LoggingType logging_;
};

}  // namespace launchdarkly::config::shared::builders
