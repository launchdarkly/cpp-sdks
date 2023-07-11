#include <chrono>
#include <cstddef>
#include <functional>
#include <iomanip>
#include <memory>
#include <optional>
#include <ostream>
#include <string>

namespace launchdarkly::common::data_sources {

template <typename DataSourceState>
class DataSourceStatusBase {
   public:
    using DateTime = std::chrono::time_point<std::chrono::system_clock>;

    /**
     * A description of an error condition that the data source encountered.
     */
    class ErrorInfo {
       public:
        using StatusCodeType = uint64_t;

        /**
         * An enumeration describing the general type of an error.
         */
        enum class ErrorKind {
            /**
             * An unexpected error, such as an uncaught exception, further
             * described by the error message.
             */
            kUnknown = 0,

            /**
             * An I/O error such as a dropped connection.
             */
            kNetworkError = 1,

            /**
             * The LaunchDarkly service returned an HTTP response with an error
             * status, available in the status code.
             */
            kErrorResponse = 2,

            /**
             * The SDK received malformed data from the LaunchDarkly service.
             */
            kInvalidData = 3,

            /**
             * The data source itself is working, but when it tried to put an
             * update into the data store, the data store failed (so the SDK may
             * not have the latest data).
             */
            kStoreError = 4
        };

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

        ErrorInfo(ErrorKind kind,
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

    /**
     * An enumerated value representing the overall current state of the data
     * source.
     */
    [[nodiscard]] DataSourceState State() const { return state_; }

    /**
     * The date/time that the value of State most recently changed.
     *
     * The meaning of this depends on the current state:
     * - For DataSourceState::kInitializing, it is the time that the SDK started
     * initializing.
     * - For DataSourceState::kValid, it is the time that the data
     * source most recently entered a valid state, after previously having been
     * DataSourceState::kInitializing or an invalid state such as
     * DataSourceState::kInterrupted.
     * - For DataSourceState::kInterrupted, it is the time that the data source
     * most recently entered an error state, after previously having been
     * DataSourceState::kValid.
     * - For DataSourceState::kShutdown, it is the time that the data source
     * encountered an unrecoverable error or that the SDK was explicitly shut
     * down.
     */
    [[nodiscard]] DateTime StateSince() const { return state_since_; }

    /**
     * Information about the last error that the data source encountered, if
     * any.
     *
     * This property should be updated whenever the data source encounters a
     * problem, even if it does not cause the state to change. For instance, if
     * a stream connection fails and the state changes to
     * DataSourceState::kInterrupted, and then subsequent attempts to restart
     * the connection also fail, the state will remain
     * DataSourceState::kInterrupted but the error information will be updated
     * each time-- and the last error will still be reported in this property
     * even if the state later becomes DataSourceState::kValid.
     */
    [[nodiscard]] std::optional<ErrorInfo> LastError() const {
        return last_error_;
    }

    DataSourceStatusBase(DataSourceState state,
                     DateTime state_since,
                     std::optional<ErrorInfo> last_error)
        : state_(state),
          state_since_(state_since),
          last_error_(std::move(last_error)) {}

    DataSourceStatusBase(DataSourceStatusBase const& status)
        : state_(status.State()),
          state_since_(status.StateSince()),
          last_error_(status.LastError()) {}

   private:
    DataSourceState state_;
    DateTime state_since_;
    std::optional<ErrorInfo> last_error_;
};

template <typename DataSourceState>
std::ostream& operator<<(std::ostream& out,
                         DataSourceStatusBase<DataSourceState> const& status) {
    std::time_t as_time_t =
        std::chrono::system_clock::to_time_t(status.StateSince());
    out << "Status(" << status.State() << ", Since("
        << std::put_time(std::gmtime(&as_time_t), "%Y-%m-%d %H:%M:%S") << ")";
    if (status.LastError()) {
        out << ", " << status.LastError().value();
    }
    out << ")";
    return out;
}

template <typename DataSourceState>
std::ostream& operator<<(
    std::ostream& out,
    typename DataSourceStatusBase<DataSourceState>::ErrorInfo const& error) {
    std::time_t as_time_t = std::chrono::system_clock::to_time_t(error.Time());
    out << "Error(" << error.Kind() << ", " << error.Message()
        << ", StatusCode(" << error.StatusCode() << "), Since("
        << std::put_time(std::gmtime(&as_time_t), "%Y-%m-%d %H:%M:%S") << "))";
    return out;
}

}  // namespace launchdarkly::data_sources
