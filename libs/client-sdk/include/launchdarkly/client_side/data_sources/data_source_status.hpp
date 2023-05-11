#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <ostream>
#include <string>

#include <boost/signals2.hpp>

#include <launchdarkly/client_side/connection.hpp>
#include <launchdarkly/client_side/data_sources/data_source_status.hpp>
#include <launchdarkly/network/detail/http_requester.hpp>

namespace launchdarkly::client_side::data_sources {

class DataSourceStatus {
   public:
    using DateTime = std::chrono::time_point<std::chrono::system_clock>;

    /**
     * Enumeration of possible data source states.
     */
    enum class DataSourceState {
        /**
         * The initial state of the data source when the SDK is being
         * initialized.
         *
         * If it encounters an error that requires it to retry initialization,
         * the state will remain at kInitializing until it either succeeds and
         * becomes kValid, or permanently fails and becomes kShutdown.
         */
        kInitializing,

        /**
         * Indicates that the data source is currently operational and has not
         * had any problems since the last time it received data.
         *
         * In streaming mode, this means that there is currently an open stream
         * connection and that at least one initial message has been received on
         * the stream. In polling mode, it means that the last poll request
         * succeeded.
         */
        kValid,

        /**
         * Indicates that the data source encountered an error that it will
         * attempt to recover from.
         *
         * In streaming mode, this means that the stream connection failed, or
         * had to be dropped due to some other error, and will be retried after
         * a backoff delay. In polling mode, it means that the last poll request
         * failed, and a new poll request will be made after the configured
         * polling interval.
         */
        kInterrupted,

        /**
         * Indicates that the application has told the SDK to stay offline.
         */
        kSetOffline,

        /**
         * Indicates that the data source has been permanently shut down.
         *
         * This could be because it encountered an unrecoverable error (for
         * instance, the LaunchDarkly service rejected the SDK key; an invalid
         * SDK key will never become valid), or because the SDK client was
         * explicitly shut down.
         */
        kShutdown

        // BackgroundDisabled,
        // TODO: A plugin of sorts would likely be required for some
        // functionality like this. kNetworkUnavailable,
    };

    /**
     * A description of an error condition that the data source encountered.
     */
    class ErrorInfo {
       public:
        using StatusCodeType = network::detail::HttpResult::StatusCode;

        /**
         * An enumeration describing the general type of an error.
         */
        enum class ErrorKind {
            /**
             * An unexpected error, such as an uncaught exception, further
             * described by the error message.
             */
            kUnknown,

            /**
             * An I/O error such as a dropped connection.
             */
            kNetworkError,

            /**
             * The LaunchDarkly service returned an HTTP response with an error
             * status, available in the status code.
             */
            kErrorResponse,

            /**
             * The SDK received malformed data from the LaunchDarkly service.
             */
            kInvalidData,

            /**
             * The data source itself is working, but when it tried to put an
             * update into the data store, the data store failed (so the SDK may
             * not have the latest data).
             */
            kStoreError
        };

        /**
         * An enumerated value representing the general category of the error.
         */
        [[nodiscard]] ErrorKind Kind() const;

        /**
         * The HTTP status code if the error was ErrorKind::kErrorResponse.
         */
        [[nodiscard]] StatusCodeType StatusCode() const;

        /**
         * Any additional human-readable information relevant to the error.
         *
         * The format is subject to change and should not be relied on
         * programmatically.
         */
        [[nodiscard]] std::string const& Message() const;

        /**
         * The date/time that the error occurred.
         */
        [[nodiscard]] DateTime Time() const;

        ErrorInfo(ErrorKind kind,
                  StatusCodeType status_code,
                  std::string message,
                  DateTime time);

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
    [[nodiscard]] DataSourceState State() const;

    /**
     * The date/time that the value of State most recently changed.
     *
     * The meaning of this depends on the current state:
     * - For DataSourceState::kInitializing, it is the time that the SDK started
     * initializing.
     * - For <see cref="DataSourceState.Valid"/>, it is the time that the data
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
    [[nodiscard]] DateTime StateSince() const;

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
    [[nodiscard]] std::optional<ErrorInfo> LastError() const;

    DataSourceStatus(DataSourceState state,
                     DateTime state_since,
                     std::optional<ErrorInfo> last_error);

   private:
    DataSourceState state_;
    DateTime state_since_;
    std::optional<ErrorInfo> last_error_;
};

/**
 * Interface for accessing and listening to the data source status.
 */
class IDataSourceStatusProvider {
   public:
    /**
     * The current status of the data source. Suitable for broadcast to
     * data source status listeners.
     */
    virtual DataSourceStatus Status() = 0;

    /**
     * Listen to changes to the data source status.
     *
     * @param handler Function which will be called with the new status.
     * @return A IConnection which can be used to stop listening to the status.
     */
    virtual std::unique_ptr<IConnection> OnDataSourceStatusChange(
        std::function<void(data_sources::DataSourceStatus status)> handler) = 0;

    virtual ~IDataSourceStatusProvider() = default;
    IDataSourceStatusProvider(IDataSourceStatusProvider const& item) = delete;
    IDataSourceStatusProvider(IDataSourceStatusProvider&& item) = delete;
    IDataSourceStatusProvider& operator=(IDataSourceStatusProvider const&) =
        delete;
    IDataSourceStatusProvider& operator=(IDataSourceStatusProvider&&) = delete;

   protected:
    IDataSourceStatusProvider() = default;
};

std::ostream& operator<<(std::ostream& out,
                         DataSourceStatus::DataSourceState const& state);

std::ostream& operator<<(std::ostream& out,
                         DataSourceStatus::ErrorInfo::ErrorKind const& kind);

std::ostream& operator<<(std::ostream& out,
                         DataSourceStatus const& status);

std::ostream& operator<<(std::ostream& out,
                         DataSourceStatus::ErrorInfo const& error);

}  // namespace launchdarkly::client_side::data_sources
