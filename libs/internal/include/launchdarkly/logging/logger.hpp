#pragma once

#include <launchdarkly/logging/log_backend.hpp>

#include <memory>
#include <ostream>
#include <sstream>

namespace launchdarkly {

/**
 * Logger to be used in SDK implementation.
 *
 * ```
 * Logger logger(std::make_shared<ConsoleBackend>(LogLevel::kInfo,
 * "Example"));
 *
 * // Use the macro for logging.
 * LD_LOG(logger, LogLevel::kInfo) << "this is a log";
 * ```
 */
class Logger {
   public:
    class LogRecord : public std::stringbuf {
        friend class Logger;

       public:
        LogRecord(LogLevel level, bool open);

        /**
         * Check if the record is open for writing. If the log level is not
         * enabled, then the record will not be open.
         * @return True if the record can be written to.
         */
        bool Open() const;

       private:
        LogLevel const level_;
        bool const open_;
    };

    /**
     * Class which allows for ostream based logging.
     *
     * Generally this should be used via the macro, but can be used directly.
     * If used directly the log level should be checked first.
     * ```
     * if(logger.enabled(LogLevel::kInfo)) {
     *  LogRecordStream stream(logger, LogLevel::kInfo);
     *  stream << "some log thing";
     *  stream << "another log thing";
     *  // The log will be written when the LogRecordStream leaves scope.
     * }
     * ```
     */
    class LogRecordStream {
       public:
        LogRecordStream(Logger const& logger, Logger::LogRecord rec);
        ~LogRecordStream();

        /**
         * Overloaded stream operator to disable conversions when the record is
         * not open.
         *
         * @tparam T The type of the object being streamed.
         * @param any The value being streamed.
         * @return Return this instance.
         */
        template <typename T>
        LogRecordStream& operator<<(T any) {
            if (rec_.open_) {
                ostream_ << any;
            }
            return *this;
        }

       private:
        Logger const& logger_;
        Logger::LogRecord rec_;
        std::ostream ostream_;
    };

    /**
     * Construct a logger with the specified back-end.
     *
     * ```
     * Logger logger(std::make_unique<ConsoleBackend>(LogLevel::kInfo,
     * "Example"));
     * ```
     *
     * @param backend The back-end to use for the logger.
     */
    Logger(std::shared_ptr<ILogBackend> backend);

    /**
     * Open a logging record.
     *
     * @param level The level for the record.
     * @return A new record, or null-opt if there are no sinks for the severity
     * level.
     */
    LogRecord OpenRecord(LogLevel level) const;

    /**
     * Push the record to the back-end.
     *
     * @param record The record to push. Should be written to before this point.
     */
    void PushRecord(LogRecord record) const;

    /**
     * Check if a given log level is enabled. This is useful if you want to do
     * some expensive operation only when logging is enabled.
     *
     * @param level The level to check.
     * @return True if the level is enabled.
     */
    bool Enabled(LogLevel level) const;

   private:
    std::shared_ptr<ILogBackend> backend_;
};

#define LD_LOG(logger, level) \
    launchdarkly::Logger::LogRecordStream(logger, logger.OpenRecord(level))

}  // namespace launchdarkly
