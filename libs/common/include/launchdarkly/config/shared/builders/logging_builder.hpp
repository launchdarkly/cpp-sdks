#pragma once

#include <launchdarkly/config/shared/built/logging.hpp>
#include <launchdarkly/logging/log_backend.hpp>

#include <variant>

namespace launchdarkly::config::shared::builders {

/**
 * Used to configure logging for the SDK.
 */
class LoggingBuilder {
   public:
    /**
     * Class for configuring built in logging using the SDKs console logger.
     */
    class BasicLogging {
       public:
        BasicLogging();

        /**
         * Set the enabled log level.
         *
         * @return A reference to this builder.
         */
        BasicLogging& Level(LogLevel level);

        /**
         * Set a tag for this logger. This tag will be included at the start
         * of log entries in square brackets.
         *
         * If the name was "LaunchDarkly", then log entries will be prefixed
         * with "[LaunchDarkly]".
         *
         * @param name
         * @return
         */
        BasicLogging& Tag(std::string name);

       private:
        LogLevel level_;
        std::string tag_;
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
     * Construct a logging builder.
     */
    LoggingBuilder() = default;

    /**
     * Construct a logging builder from a custom logging builder.
     * @param custom The custom logging builder to construct a builder from.
     */
    LoggingBuilder(CustomLogging custom);

    /**
     * Construct a logging builder from a basic logging builder.
     * @param basic The basic logging builder to construct a builder from.
     */
    LoggingBuilder(BasicLogging basic);

    /**
     * Construct a logging builder from a no logging builder.
     * @param no The no logging builder to construct a builder from.
     */
    LoggingBuilder(NoLogging no);

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

    /**
     * Build a logger configuration. Intended for use by the SDK implementation.
     *
     * @return A built logging configuration.
     */
    built::Logging Build() const;

   private:
    LoggingType logging_;
};

}  // namespace launchdarkly::config::shared::builders
