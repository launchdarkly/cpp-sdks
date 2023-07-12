#include <chrono>
#include <optional>

#include <launchdarkly/data_sources/data_source_status_error_info.hpp>
#include <launchdarkly/data_sources/data_source_status_error_kind.hpp>

// Common is included in the namespace to disambiguate from client/server
// for backward compatibility.
namespace launchdarkly::common::data_sources {

template <typename DataSourceState>
class DataSourceStatusBase {
   public:
    using ErrorKind = DataSourceStatusErrorKind;
    using ErrorInfo = DataSourceStatusErrorInfo;
    using DateTime = std::chrono::time_point<std::chrono::system_clock>;

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
     * - For DataSourceState::kShutdown (client-side) or DataSourceState::kOff
     * (server-side), it is the time that the data source encountered an
     * unrecoverable error or that the SDK was explicitly shut down.
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

    virtual ~DataSourceStatusBase() = default;
    DataSourceStatusBase(DataSourceStatusBase const& item) = default;
    DataSourceStatusBase(DataSourceStatusBase&& item) = default;
    DataSourceStatusBase& operator=(DataSourceStatusBase const&) = delete;
    DataSourceStatusBase& operator=(DataSourceStatusBase&&) = delete;

   private:
    DataSourceState state_;
    DateTime state_since_;
    std::optional<ErrorInfo> last_error_;
};

}  // namespace launchdarkly::common::data_sources
