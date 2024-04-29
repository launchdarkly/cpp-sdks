#pragma once

#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <unordered_map>

#include <launchdarkly/context.hpp>
#include <launchdarkly/value.hpp>

#include <launchdarkly/client_side/data_source_status.hpp>
#include <launchdarkly/client_side/flag_notifier.hpp>
#include <launchdarkly/config/client.hpp>
#include <launchdarkly/data/evaluation_detail.hpp>

namespace launchdarkly::client_side {

/**
 * The default amount of time Client::WaitForInitialization will wait up to
 * before returning control to the caller.
 */
constexpr static std::chrono::seconds kDefaultInitializationTimeout{5};

/**
 *  Interface for the standard SDK client methods and properties.
 */
class IClient {
   public:
    /**
     * Represents the key of a feature flag.
     */
    using FlagKey = std::string;

    /**
     * @deprecated See Initialize instead.
     *
     * Connects the client to LaunchDarkly's flag delivery endpoints.
     *
     * If StartAsync isn't called, the client is able to post events but is
     * unable to obtain flag data.
     *
     * The returned future will resolve to true or false based on the logic
     * outlined on @ref Initialized.
     */
    virtual std::future<bool> StartAsync() = 0;

    /** Initializes the client with fresh flag data from LaunchDarkly.
     *
     * If Initialize is not called, the client will use application-provided
     * default values or cached flag values if a persistent store is in use.
     *
     * To block until the connection is established, call WaitForInitialization
     * after calling Initialize.
     *
     * The client may still post events to LaunchDarkly even if Initialize isn't
     * called.
     *
     * It is not safe to call Initialize more than once. If the client
     * encounters an unrecoverable error, destroy it and create a new client.
     *
     */
    virtual void Initialize() = 0;

    /**
     * This is a convenience function that blocks the calling thread until the
     * earliest of the following occurs:
     * (1) The client has obtained fresh flag data
     * (2) The client has encountered an unrecoverable error
     * (3) The timeout has elapsed
     *
     * If the client is in offline mode or has already received fresh flag data,
     * then WaitForInitialization returns immediately.
     *
     * The default timeout is 5 seconds, meaning that if you call this method
     * with no parameter, it will block the calling thread for up to 5 seconds.
     *
     * This method is not safe for concurrent use.
     *
     * Although it is possible to call WaitForInitialization more than once
     * serially, for example in a loop, it is likely a better idea to
     * register a DataSourceStatus listener to monitor the connection status
     * directly.
     *
     * @code
     * client.Initialize(); // client begins connecting in background
     * bool initialized = client.WaitForInitialization(); // blocks for up to 5s
     * if (initialized) {
     *    // client has fresh flag data
     * } else {
     *    // client encountered an unrecoverable error, is still connecting,
     *    // or hit the 5 second timeout.
     * }
     * @endcode
     *
     * @param timeout How long to wait for fresh flag data before returning.
     * Defaults to 5 seconds.
     * @return True if the client has fresh flag data (case 1). False if the
     * client encountered a unrecoverable error or the timeout elapsed (cases 2
     * and 3). To obtain more granular information about failure, setup a
     * DataSourceStatus listener.
     */
    virtual bool WaitForInitialization(
        std::chrono::milliseconds timeout = kDefaultInitializationTimeout) = 0;

    /**
     * Returns a boolean value indicating LaunchDarkly connection and flag state
     * within the client.
     *
     * When you first start the client, once StartAsync has completed,
     * Initialized should return true if and only if either 1. it connected to
     * LaunchDarkly and successfully retrieved flags, or 2. it started in
     * offline mode so there's no need to connect to LaunchDarkly. If the client
     * timed out trying to connect to LD, then Initialized returns false (even
     * if we do have cached flags). If the client connected and got a 401 error,
     * Initialized will return false. This serves the purpose of letting the
     * app know that there was a problem of some kind.
     *
     * @return True if the client is initialized.
     */
    [[nodiscard]] virtual bool Initialized() const = 0;

    /**
     * Returns a map from feature flag keys to feature
     * flag values for the current context.
     *
     * This method will not send analytics events back to LaunchDarkly.
     *
     * @return A map from feature flag keys to values for the current context.
     */
    [[nodiscard]] virtual std::unordered_map<FlagKey, Value> AllFlags()
        const = 0;

    /**
     * Tracks that the current context performed an event for the given event
     * name, and associates it with a numeric metric value.
     *
     * @param event_name The name of the event.
     * @param data A JSON value containing additional data associated with the
     * event.
     * @param metric_value this value is used by the LaunchDarkly
     * experimentation feature in numeric custom metrics, and will also be
     * returned as part of the custom event for Data Export
     */
    virtual void Track(std::string event_name,
                       Value data,
                       double metric_value) = 0;

    /**
     * Tracks that the current context performed an event for the given event
     * name, with additional JSON data.
     *
     * @param event_name The name of the event.
     * @param data A JSON value containing additional data associated with the
     * event.
     */
    virtual void Track(std::string event_name, Value data) = 0;

    /**
     * Tracks that the current context performed an event for the given event
     * name.
     *
     * @param event_name The name of the event.
     */
    virtual void Track(std::string event_name) = 0;

    /**
     * Tells the client that all pending analytics events (if any) should be
     * delivered as soon as possible.
     */
    virtual void FlushAsync() = 0;

    /**
     * Changes the current evaluation context, requests flags for that context
     * from LaunchDarkly if online, and generates an analytics event to
     * tell LaunchDarkly about the context.
     *
     * Only one IdentifyAsync can be in progress at once; calling it
     * concurrently invokes undefined behavior.
     *
     * To block until the identify operation is complete, call wait() on
     * the returned future.
     *
     * The returned future will resolve to true or false based on the logic
     * outlined on @ref Initialized.
     *
     * @param context The new evaluation context.
     */

    virtual std::future<bool> IdentifyAsync(Context context) = 0;

    /**
     * Returns the boolean value of a feature flag for a given flag key.
     *
     * @param key The unique feature key for the feature flag.
     * @param default_value The default value of the flag.
     * @return The variation for the selected context, or default_value if the
     * flag is disabled in the LaunchDarkly control panel
     */
    virtual bool BoolVariation(FlagKey const& key, bool default_value) = 0;

    /**
     * Returns the boolean value of a feature flag for a given flag key, in an
     * object that also describes the way the value was determined.
     *
     * @param key The unique feature key for the feature flag.
     * @param default_value The default value of the flag.
     * @return An evaluation detail object.
     */
    virtual EvaluationDetail<bool> BoolVariationDetail(FlagKey const& key,
                                                       bool default_value) = 0;

    /**
     * Returns the string value of a feature flag for a given flag key.
     *
     * @param key The unique feature key for the feature flag.
     * @param default_value The default value of the flag.
     * @return The variation for the selected context, or default_value if the
     * flag is disabled in the LaunchDarkly control panel
     */
    virtual std::string StringVariation(FlagKey const& key,
                                        std::string default_value) = 0;

    /**
     * Returns the string value of a feature flag for a given flag key, in an
     * object that also describes the way the value was determined.
     *
     * @param key The unique feature key for the feature flag.
     * @param default_value The default value of the flag.
     * @return An evaluation detail object.
     */
    virtual EvaluationDetail<std::string> StringVariationDetail(
        FlagKey const& key,
        std::string default_value) = 0;

    /**
     * Returns the double value of a feature flag for a given flag key.
     *
     * @param key The unique feature key for the feature flag.
     * @param default_value The default value of the flag.
     * @return The variation for the selected context, or default_value if the
     * flag is disabled in the LaunchDarkly control panel
     */
    virtual double DoubleVariation(FlagKey const& key,
                                   double default_value) = 0;

    /**
     * Returns the double value of a feature flag for a given flag key, in an
     * object that also describes the way the value was determined.
     *
     * @param key The unique feature key for the feature flag.
     * @param default_value The default value of the flag.
     * @return An evaluation detail object.
     */
    virtual EvaluationDetail<double> DoubleVariationDetail(
        FlagKey const& key,
        double default_value) = 0;

    /**
     * Returns the int value of a feature flag for a given flag key.
     *
     * @param key The unique feature key for the feature flag.
     * @param default_value The default value of the flag.
     * @return The variation for the selected context, or default_value if the
     * flag is disabled in the LaunchDarkly control panel
     */
    virtual int IntVariation(FlagKey const& key, int default_value) = 0;

    /**
     * Returns the int value of a feature flag for a given flag key, in an
     * object that also describes the way the value was determined.
     *
     * @param key The unique feature key for the feature flag.
     * @param default_value The default value of the flag.
     * @return An evaluation detail object.
     */
    virtual EvaluationDetail<int> IntVariationDetail(FlagKey const& key,
                                                     int default_value) = 0;

    /**
     * Returns the JSON value of a feature flag for a given flag key.
     *
     * @param key The unique feature key for the feature flag.
     * @param default_value The default value of the flag.
     * @return The variation for the selected context, or default_value if the
     * flag is disabled in the LaunchDarkly control panel
     */
    virtual Value JsonVariation(FlagKey const& key, Value default_value) = 0;

    /**
     * Returns the JSON value of a feature flag for a given flag key, in an
     * object that also describes the way the value was determined.
     *
     * @param key The unique feature key for the feature flag.
     * @param default_value The default value of the flag.
     * @return An evaluation detail object.
     */
    virtual EvaluationDetail<Value> JsonVariationDetail(
        FlagKey const& key,
        Value default_value) = 0;

    /**
     * Returns an interface which provides methods for subscribing to data
     * source status.
     * @return A data source status provider.
     */
    virtual data_sources::IDataSourceStatusProvider& DataSourceStatus() = 0;

    /**
     * Returns an interface which provides methods for subscribing to data
     * flag changes.
     * @return A flag notifier.
     */
    virtual flag_manager::IFlagNotifier& FlagNotifier() = 0;

    virtual ~IClient() = default;
    IClient(IClient const& item) = delete;
    IClient(IClient&& item) = delete;
    IClient& operator=(IClient const&) = delete;
    IClient& operator=(IClient&&) = delete;

   protected:
    IClient() = default;
};

class Client : public IClient {
   public:
    Client(Config config, Context context);

    Client(Client&&) = delete;
    Client(Client const&) = delete;
    Client& operator=(Client) = delete;
    Client& operator=(Client&& other) = delete;

    std::future<bool> StartAsync() override;

    void Initialize() override;

    bool WaitForInitialization(std::chrono::milliseconds timeout =
                                   kDefaultInitializationTimeout) override;

    [[nodiscard]] bool Initialized() const override;

    using FlagKey = std::string;
    [[nodiscard]] std::unordered_map<FlagKey, Value> AllFlags() const override;

    void Track(std::string event_name,
               Value data,
               double metric_value) override;

    void Track(std::string event_name, Value data) override;

    void Track(std::string event_name) override;

    void FlushAsync() override;

    std::future<bool> IdentifyAsync(Context context) override;

    bool BoolVariation(FlagKey const& key, bool default_value) override;

    EvaluationDetail<bool> BoolVariationDetail(FlagKey const& key,
                                               bool default_value) override;

    std::string StringVariation(FlagKey const& key,
                                std::string default_value) override;

    EvaluationDetail<std::string> StringVariationDetail(
        FlagKey const& key,
        std::string default_value) override;

    double DoubleVariation(FlagKey const& key, double default_value) override;

    EvaluationDetail<double> DoubleVariationDetail(
        FlagKey const& key,
        double default_value) override;

    int IntVariation(FlagKey const& key, int default_value) override;

    EvaluationDetail<int> IntVariationDetail(FlagKey const& key,
                                             int default_value) override;

    Value JsonVariation(FlagKey const& key, Value default_value) override;

    EvaluationDetail<Value> JsonVariationDetail(FlagKey const& key,
                                                Value default_value) override;

    data_sources::IDataSourceStatusProvider& DataSourceStatus() override;

    flag_manager::IFlagNotifier& FlagNotifier() override;

    /**
     * Returns the version of the SDK.
     * @return String representing version of the SDK.
     */
    [[nodiscard]] static char const* Version();

   private:
    inline static char const* const kVersion =
        "3.4.3";  // {x-release-please-version}
    std::unique_ptr<IClient> client;
};

}  // namespace launchdarkly::client_side
