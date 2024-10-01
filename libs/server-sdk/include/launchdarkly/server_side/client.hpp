#pragma once

#include <launchdarkly/context.hpp>
#include <launchdarkly/data/evaluation_detail.hpp>
#include <launchdarkly/server_side/config/config.hpp>
#include <launchdarkly/value.hpp>

#include <launchdarkly/server_side/all_flags_state.hpp>
#include <launchdarkly/server_side/data_source_status.hpp>

#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side {
/**
 *  Interface for the standard SDK client methods and properties.
 */
class IClient {
   public:
    /**
     * Represents the key of a feature flag.
     */
    using FlagKey = std::string;

    /** Connects the client to LaunchDarkly's flag delivery endpoints.
     *
     * StartAsync must be called once for the SDK to start receiving flag data.
     * It does not need to be called more than one time.
     *
     * The returned future will resolve to true or false based on the logic
     * outlined on @ref Initialized.
     *
     * Blocking indefinitely on the future (e.g. by calling .get() or
     * .wait()) is highly discouraged. Instead, use a method that takes a
     * timeout like .wait_for() or .wait_until(), or do not wait.
     *
     * Otherwise, the application may hang indefinitely if the client cannot
     * connect to LaunchDarkly.
     *
     * While the client is connecting asynchronously, it is safe to call
     * variation methods, which will return application-defined default values.
     *
     * The client will always continue to attempt to connect asynchronously
     * after being started unless it encounters an unrecoverable error. The
     * returned promise timing out does not affect this behavior.
     */
    virtual std::future<bool> StartAsync() = 0;

    /**
     * Returns a boolean value indicating LaunchDarkly connection and flag state
     * within the client.
     *
     * When you first start the client, once StartAsync has completed,
     * Initialized should return true if and only if either 1. it connected to
     * LaunchDarkly and successfully retrieved flags, or 2. it started in
     * offline mode so there's no need to connect to LaunchDarkly.
     *
     * If the client timed out trying to connect to LD, then Initialized returns
     * false (even if we do have cached flags). If the client connected and
     * got a 401 error, Initialized will return false.
     *
     * This serves the purpose of letting the
     * app know that there was a problem of some kind.
     *
     * @return True if the client is initialized.
     */
    [[nodiscard]] virtual bool Initialized() const = 0;

    /**
     * Evaluates all flags for a context, returning a data structure containing
     * the results and additional flag metadata.
     *
     * The method's behavior can be controlled by passing a combination of
     * one or more options.
     *
     * A common use-case for AllFlagsState is to generate data suitable for
     * bootstrapping the client-side JavaScript SDK.
     *
     * This method will not send analytics events back to LaunchDarkly.
     *
     * @param context  The context against which all flags will be
     * evaluated.
     * @param options A combination of one or more options. Omitting this
     * argument is equivalent to passing AllFlagsState::Options::Default.
     * @return An AllFlagsState data structure.
     */
    [[nodiscard]] virtual class AllFlagsState AllFlagsState(
        Context const& context,
        AllFlagsState::Options options = AllFlagsState::Options::Default) = 0;

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
    virtual void Track(Context const& ctx,
                       std::string event_name,
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
    virtual void Track(Context const& ctx,
                       std::string event_name,
                       Value data) = 0;

    /**
     * Tracks that the current context performed an event for the given event
     * name.
     *
     * @param event_name The name of the event.
     */
    virtual void Track(Context const& ctx, std::string event_name) = 0;

    /**
     * Tells the client that all pending analytics events (if any) should be
     * delivered as soon as possible.
     */
    virtual void FlushAsync() = 0;

    /**
     * Generates an identify event for a context.
     *
     * @param context The new evaluation context.
     */

    virtual void Identify(Context context) = 0;

    /**
     * Returns the boolean value of a feature flag for a given flag key.
     *
     * @param key The unique feature key for the feature flag.
     * @param default_value The default value of the flag.
     * @return The variation for the selected context, or default_value if the
     * flag is disabled in the LaunchDarkly control panel
     */
    virtual bool BoolVariation(Context const& ctx,
                               FlagKey const& key,
                               bool default_value) = 0;

    /**
     * Returns the boolean value of a feature flag for a given flag key, in an
     * object that also describes the way the value was determined.
     *
     * @param key The unique feature key for the feature flag.
     * @param default_value The default value of the flag.
     * @return An evaluation detail object.
     */
    virtual EvaluationDetail<bool> BoolVariationDetail(Context const& ctx,
                                                       FlagKey const& key,
                                                       bool default_value) = 0;

    /**
     * Returns the string value of a feature flag for a given flag key.
     *
     * @param key The unique feature key for the feature flag.
     * @param default_value The default value of the flag.
     * @return The variation for the selected context, or default_value if the
     * flag is disabled in the LaunchDarkly control panel
     */
    virtual std::string StringVariation(Context const& ctx,
                                        FlagKey const& key,
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
        Context const& ctx,
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
    virtual double DoubleVariation(Context const& ctx,
                                   FlagKey const& key,
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
        Context const& ctx,
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
    virtual int IntVariation(Context const& ctx,
                             FlagKey const& key,
                             int default_value) = 0;

    /**
     * Returns the int value of a feature flag for a given flag key, in an
     * object that also describes the way the value was determined.
     *
     * @param key The unique feature key for the feature flag.
     * @param default_value The default value of the flag.
     * @return An evaluation detail object.
     */
    virtual EvaluationDetail<int> IntVariationDetail(Context const& ctx,
                                                     FlagKey const& key,
                                                     int default_value) = 0;

    /**
     * Returns the JSON value of a feature flag for a given flag key.
     *
     * @param key The unique feature key for the feature flag.
     * @param default_value The default value of the flag.
     * @return The variation for the selected context, or default_value if the
     * flag is disabled in the LaunchDarkly control panel
     */
    virtual Value JsonVariation(Context const& ctx,
                                FlagKey const& key,
                                Value default_value) = 0;

    /**
     * Returns the JSON value of a feature flag for a given flag key, in an
     * object that also describes the way the value was determined.
     *
     * @param key The unique feature key for the feature flag.
     * @param default_value The default value of the flag.
     * @return An evaluation detail object.
     */
    virtual EvaluationDetail<Value> JsonVariationDetail(
        Context const& ctx,
        FlagKey const& key,
        Value default_value) = 0;

    /**
     * Returns an interface which provides methods for subscribing to data
     * source status.
     * @return A data source status provider.
     */
    virtual IDataSourceStatusProvider& DataSourceStatus() = 0;

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
    Client(Config config);

    std::future<bool> StartAsync() override;

    [[nodiscard]] bool Initialized() const override;

    using FlagKey = std::string;
    [[nodiscard]] class AllFlagsState AllFlagsState(
        Context const& context,
        enum AllFlagsState::Options options =
            AllFlagsState::Options::Default) override;

    void Track(Context const& ctx,
               std::string event_name,
               Value data,
               double metric_value) override;

    void Track(Context const& ctx, std::string event_name, Value data) override;

    void Track(Context const& ctx, std::string event_name) override;

    void FlushAsync() override;

    void Identify(Context context) override;

    bool BoolVariation(Context const& ctx,
                       FlagKey const& key,
                       bool default_value) override;

    EvaluationDetail<bool> BoolVariationDetail(Context const& ctx,
                                               FlagKey const& key,
                                               bool default_value) override;

    std::string StringVariation(Context const& ctx,
                                FlagKey const& key,
                                std::string default_value) override;

    EvaluationDetail<std::string> StringVariationDetail(
        Context const& ctx,
        FlagKey const& key,
        std::string default_value) override;

    double DoubleVariation(Context const& ctx,
                           FlagKey const& key,
                           double default_value) override;

    EvaluationDetail<double> DoubleVariationDetail(
        Context const& ctx,
        FlagKey const& key,
        double default_value) override;

    int IntVariation(Context const& ctx,
                     FlagKey const& key,
                     int default_value) override;

    EvaluationDetail<int> IntVariationDetail(Context const& ctx,
                                             FlagKey const& key,
                                             int default_value) override;

    Value JsonVariation(Context const& ctx,
                        FlagKey const& key,
                        Value default_value) override;

    EvaluationDetail<Value> JsonVariationDetail(Context const& ctx,
                                                FlagKey const& key,
                                                Value default_value) override;

    IDataSourceStatusProvider& DataSourceStatus() override;

    /**
     * Returns the version of the SDK.
     * @return String representing version of the SDK.
     */
    [[nodiscard]] static char const* Version();

   private:
    inline static char const* const kVersion =
        "3.6.1";  // {x-release-please-version}
    std::unique_ptr<IClient> client;
};

}  // namespace launchdarkly::server_side
