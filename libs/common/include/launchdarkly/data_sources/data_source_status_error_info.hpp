#pragma once

#include <launchdarkly/data_sources/data_source_status_error_kind.hpp>

#include <cstdint>

namespace launchdarkly::common::data_sources {

/**
 * A description of an error condition that the data source encountered.
 */
class DataSourceStatusErrorInfo {
   public:
    using StatusCodeType = uint64_t;
    using ErrorKind = DataSourceStatusErrorKind;
    using DateTime = std::chrono::time_point<std::chrono::system_clock>;

    /**
     * An enumerated value representing the general category of the error.
     */
    [[nodiscard]] ErrorKind Kind() const { return kind_; }

    /**
     * The HTTP status code if the error was ErrorKind::kErrorResponse.
     */
    [[nodiscard]] StatusCodeType StatusCode() const { return status_code_; }

    /**
     * Any additional human-readable information relevant to the error.
     *
     * The format is subject to change and should not be relied on
     * programmatically.
     */
    [[nodiscard]] std::string const& Message() const { return message_; }

    /**
     * The date/time that the error occurred.
     */
    [[nodiscard]] DateTime Time() const { return time_; }

    DataSourceStatusErrorInfo(ErrorKind kind,
                              StatusCodeType status_code,
                              std::string message,
                              DateTime time)
        : kind_(kind),
          status_code_(status_code),
          message_(std::move(message)),
          time_(time) {}

   private:
    ErrorKind kind_;
    StatusCodeType status_code_;
    std::string message_;
    DateTime time_;
};

std::ostream& operator<<(std::ostream& out,
                         DataSourceStatusErrorInfo const& error);

}  // namespace launchdarkly::common::data_sources
