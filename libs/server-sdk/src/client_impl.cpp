
#include <chrono>

#include <optional>
#include <utility>

#include "client_impl.hpp"

#include "all_flags_state/all_flags_state_builder.hpp"
#include "data_sources/null_data_source.hpp"
#include "data_sources/polling_data_source.hpp"
#include "data_sources/streaming_data_source.hpp"
#include "data_store/memory_store.hpp"

#include <launchdarkly/encoding/sha_256.hpp>
#include <launchdarkly/events/asio_event_processor.hpp>
#include <launchdarkly/events/data/common_events.hpp>
#include <launchdarkly/events/null_event_processor.hpp>
#include <launchdarkly/logging/console_backend.hpp>
#include <launchdarkly/logging/null_logger.hpp>

namespace launchdarkly::server_side {

// The ASIO implementation assumes that the io_context will be run from a
// single thread, and applies several optimisations based on this
// assumption.
auto const kAsioConcurrencyHint = 1;

// Client's destructor attempts to gracefully shut down the datasource
// connection in this amount of time.
auto const kDataSourceShutdownWait = std::chrono::milliseconds(100);

using config::shared::ServerSDK;
using launchdarkly::config::shared::built::DataSourceConfig;
using launchdarkly::config::shared::built::HttpProperties;
using launchdarkly::server_side::data_sources::DataSourceStatus;

static std::shared_ptr<::launchdarkly::data_sources::IDataSource>
MakeDataSource(HttpProperties const& http_properties,
               Config const& config,
               boost::asio::any_io_executor const& executor,
               data_sources::IDataSourceUpdateSink& flag_updater,
               data_sources::DataSourceStatusManager& status_manager,
               Logger& logger) {
    if (config.Offline()) {
        return std::make_shared<data_sources::NullDataSource>(executor,
                                                              status_manager);
    }

    auto builder = HttpPropertiesBuilder(http_properties);

    auto data_source_properties = builder.Build();

    if (config.DataSourceConfig().method.index() == 0) {
        // TODO: use initial reconnect delay.
        return std::make_shared<
            launchdarkly::server_side::data_sources::StreamingDataSource>(
            config.ServiceEndpoints(), config.DataSourceConfig(),
            data_source_properties, executor, flag_updater, status_manager,
            logger);
    }
    return std::make_shared<
        launchdarkly::server_side::data_sources::PollingDataSource>(
        config.ServiceEndpoints(), config.DataSourceConfig(),
        data_source_properties, executor, flag_updater, status_manager, logger);
}

static Logger MakeLogger(config::shared::built::Logging const& config) {
    if (config.disable_logging) {
        return {std::make_shared<logging::NullLoggerBackend>()};
    }
    if (config.backend) {
        return {config.backend};
    }
    return {
        std::make_shared<logging::ConsoleBackend>(config.level, config.tag)};
}

ClientImpl::ClientImpl(Config config, std::string const& version)
    : config_(config),
      http_properties_(
          HttpPropertiesBuilder(config.HttpProperties())
              .Header("user-agent", "CPPClient/" + version)
              .Header("authorization", config.SdkKey())
              .Header("x-launchdarkly-tags", config.ApplicationTag())
              .Build()),
      logger_(MakeLogger(config.Logging())),
      ioc_(kAsioConcurrencyHint),
      work_(boost::asio::make_work_guard(ioc_)),
      memory_store_(),
      data_source_(MakeDataSource(http_properties_,
                                  config_,
                                  ioc_.get_executor(),
                                  memory_store_,
                                  status_manager_,
                                  logger_)),
      event_processor_(nullptr),
      evaluator_(logger_, memory_store_),
      events_default_{config_.Offline(), EventFactory::WithoutReasons()},
      events_with_reasons_{config_.Offline(), EventFactory::WithReasons()} {
    if (config.Events().Enabled() && !config.Offline()) {
        event_processor_ =
            std::make_unique<events::AsioEventProcessor<ServerSDK>>(
                ioc_.get_executor(), config.ServiceEndpoints(), config.Events(),
                http_properties_, logger_);
    } else {
        event_processor_ = std::make_unique<events::NullEventProcessor>();
    }

    run_thread_ = std::move(std::thread([&]() { ioc_.run(); }));
}

// TODO: audit if this is correct for server
// Was an attempt made to initialize the data source, and did that attempt
// succeed? The data source being connected, or not being connected due to
// offline mode, both represent successful terminal states.
static bool IsInitializedSuccessfully(DataSourceStatus::DataSourceState state) {
    return state == DataSourceStatus::DataSourceState::kValid;
}

// TODO: audit if this is correct for server
// Was any attempt made to initialize the data source (with a successful or
// permanent failure outcome?)
static bool IsInitialized(DataSourceStatus::DataSourceState state) {
    return IsInitializedSuccessfully(state) ||
           (state == DataSourceStatus::DataSourceState::kOff);
}

void ClientImpl::Identify(Context context) {
    if (events_default_.disabled) {
        return;
    }

    event_processor_->SendAsync(
        events_default_.factory.Identify(std::move(context)));
}

std::future<bool> ClientImpl::StartAsyncInternal(
    std::function<bool(DataSourceStatus::DataSourceState)> result_predicate) {
    auto pr = std::make_shared<std::promise<bool>>();
    auto fut = pr->get_future();

    status_manager_.OnDataSourceStatusChangeEx(
        [result_predicate, pr](data_sources::DataSourceStatus status) {
            auto state = status.State();
            if (IsInitialized(state)) {
                pr->set_value(result_predicate(status.State()));
                return true; /* delete this change listener since the
                                desired state was reached */
            }
            return false; /* keep the change listener */
        });

    data_source_->Start();

    return fut;
}

std::future<bool> ClientImpl::StartAsync() {
    return StartAsyncInternal(IsInitializedSuccessfully);
}

bool ClientImpl::Initialized() const {
    return IsInitializedSuccessfully(status_manager_.Status().State());
}

AllFlagsState ClientImpl::AllFlagsState(Context const& context,
                                        AllFlagsState::Options options) {
    std::unordered_map<Client::FlagKey, Value> result;

    if (!Initialized()) {
        if (memory_store_.Initialized()) {
            LD_LOG(logger_, LogLevel::kWarn)
                << "AllFlagsState() called before client has finished "
                   "initializing; using last known values from data store";
        } else {
            LD_LOG(logger_, LogLevel::kWarn)
                << "AllFlagsState() called before client has finished "
                   "initializing. Data store not available. Returning empty "
                   "state";
            return {};
        }
    }

    AllFlagsStateBuilder builder{options};

    for (auto const& [k, v] : memory_store_.AllFlags()) {
        if (!v || !v->item) {
            continue;
        }

        auto const& flag = *(v->item);

        if (IsSet(options, AllFlagsState::Options::ClientSideOnly) &&
            !flag.clientSideAvailability.usingEnvironmentId) {
            continue;
        }

        EvaluationDetail<Value> detail = evaluator_.Evaluate(flag, context);

        bool in_experiment = IsExperimentationEnabled(flag, detail.Reason());
        builder.AddFlag(k, detail.Value(),
                        AllFlagsState::State{
                            flag.Version(), detail.VariationIndex(),
                            detail.Reason(), flag.trackEvents || in_experiment,
                            in_experiment, flag.debugEventsUntilDate});
    }

    return builder.Build();
}

void ClientImpl::TrackInternal(Context const& ctx,
                               std::string event_name,
                               std::optional<Value> data,
                               std::optional<double> metric_value) {
    event_processor_->SendAsync(events::ServerTrackEventParams{
        {std::chrono::system_clock::now(), std::move(event_name),
         ctx.KindsToKeys(), std::move(data), metric_value},
        ctx});
}

void ClientImpl::Track(Context const& ctx,
                       std::string event_name,
                       Value data,
                       double metric_value) {
    this->TrackInternal(ctx, std::move(event_name), std::move(data),
                        metric_value);
}

void ClientImpl::Track(Context const& ctx, std::string event_name, Value data) {
    this->TrackInternal(ctx, std::move(event_name), std::move(data),
                        std::nullopt);
}

void ClientImpl::Track(Context const& ctx, std::string event_name) {
    this->TrackInternal(ctx, std::move(event_name), std::nullopt, std::nullopt);
}

void ClientImpl::FlushAsync() {
    event_processor_->FlushAsync();
}

// void ClientImpl::LogVariationCall(std::string const& key,
//                                   data_store::FlagDescriptor const*
//                                   flag_desc, bool initialized) {
//     bool flag_found = flag_desc != nullptr && flag_desc->item.has_value();
//
//     if (initialized) {
//         if (!flag_found) {
//             LD_LOG(logger_, LogLevel::kInfo) << "Unknown feature flag " <<
//             key
//                                              << "; returning default value";
//         }
//     } else {
//         if (flag_found) {
//             LD_LOG(logger_, LogLevel::kInfo)
//                 << "LaunchDarkly client has not yet been initialized; using "
//                    "last "
//                    "known flag rules from data store";
//         } else {
//             LD_LOG(logger_, LogLevel::kInfo)
//                 << "LaunchDarkly client has not yet been initialize;
//                 returning "
//                    "default value";
//         }
//     }
// }

// TO evaluate internally:
// 1. Try to get the flag from the memory store.
// 2. Conditionally emit a log message based on the result of step 1 and the
// initialization status of the client.
// 3. Evaluate the flag.
// 4. If the detail value is NULL, then:
//    - Substitute in the default value,
//    - Propagate the error reason
//    - wipe out the variation index
// 3. Conditionally emit an event based on 2:
//    a) If the flag was found: emit an eval event.
//    b) If the flag wasn't found: emit an "unknown flag" event.
// 4. In detail methods, do the typechecking. If the type is correct, then go
// ahead and return it.
//    Otherwise, substitute in the default value with the kWrongType.

bool FlagNotFound(
    std::shared_ptr<data_store::FlagDescriptor> const& flag_desc) {
    return !flag_desc || !flag_desc->item;
}

std::optional<enum EvaluationReason::ErrorKind> ClientImpl::PreEvaluationChecks(
    Context const& context) {
    if (!memory_store_.Initialized()) {
        return EvaluationReason::ErrorKind::kClientNotReady;
    }
    if (!context.Valid()) {
        return EvaluationReason::ErrorKind::kUserNotSpecified;
    }
    return std::nullopt;
}

EvaluationDetail<Value> ClientImpl::PostEvaluation(
    std::string const& key,
    Context const& context,
    Value const& default_value,
    std::variant<enum EvaluationReason::ErrorKind, EvaluationDetail<Value>>
        result,
    EventScope& event_scope,
    std::optional<data_model::Flag> const& flag) {
    if (!event_scope.disabled) {
        if (auto const error(
                std::get_if<enum EvaluationReason::ErrorKind>(&result));
            error) {
            if (*error == EvaluationReason::ErrorKind::kFlagNotFound) {
                event_processor_->SendAsync(event_scope.factory.UnknownFlag(
                    key, context,
                    EvaluationDetail<Value>{*error, default_value},
                    default_value));
            }
        } else {
            event_processor_->SendAsync(event_scope.factory.Eval(
                key, context, flag, std::get<EvaluationDetail<Value>>(result),
                default_value, std::nullopt));
        }
    }

    if (auto const error(
            std::get_if<enum EvaluationReason::ErrorKind>(&result));
        error) {
        return EvaluationDetail<Value>{*error, default_value};
    }

    EvaluationDetail<Value> res = std::get<EvaluationDetail<Value>>(result);
    if (res.Value().IsNull()) {
        return EvaluationDetail<Value>{default_value, res.VariationIndex(),
                                       res.Reason()};
    }
    return res;
}

EvaluationDetail<Value> ClientImpl::VariationInternal(
    Context const& context,
    IClient::FlagKey const& key,
    Value const& default_value,
    EventScope& scope) {
    if (auto error = PreEvaluationChecks(context)) {
        return PostEvaluation(key, context, default_value, *error, scope,
                              std::nullopt);
    }

    auto flag_rule = memory_store_.GetFlag(key);

    if (FlagNotFound(flag_rule)) {
        return PostEvaluation(key, context, default_value,
                              EvaluationReason::ErrorKind::kFlagNotFound, scope,
                              std::nullopt);
    }

    EvaluationDetail<Value> result =
        evaluator_.Evaluate(*flag_rule->item, context);
    return PostEvaluation(key, context, default_value, result, scope,
                          flag_rule.get()->item);
}

EvaluationDetail<Value> ClientImpl::VariationDetail(
    Context const& ctx,
    IClient::FlagKey const& key,
    Value const& default_value) {
    return VariationInternal(ctx, key, default_value, events_with_reasons_);
}

Value ClientImpl::Variation(Context const& ctx,
                            IClient::FlagKey const& key,
                            Value const& default_value) {
    return *VariationInternal(ctx, key, default_value, events_default_);
}

EvaluationDetail<bool> ClientImpl::BoolVariationDetail(
    Context const& ctx,
    IClient::FlagKey const& key,
    bool default_value) {
    auto result = VariationDetail(ctx, key, default_value);
    if (result.Value().IsBool()) {
        return EvaluationDetail<bool>{result.Value(), result.VariationIndex(),
                                      result.Reason()};
    }
    return EvaluationDetail<bool>{EvaluationReason::ErrorKind::kWrongType,
                                  default_value};
}

bool ClientImpl::BoolVariation(Context const& ctx,
                               IClient::FlagKey const& key,
                               bool default_value) {
    auto result = Variation(ctx, key, default_value);
    if (result.IsBool()) {
        return result;
    }
    return default_value;
}

EvaluationDetail<std::string> ClientImpl::StringVariationDetail(
    Context const& ctx,
    ClientImpl::FlagKey const& key,
    std::string default_value) {
    auto result = VariationDetail(ctx, key, default_value);
    if (result.Value().IsString()) {
        return EvaluationDetail<std::string>{
            result.Value(), result.VariationIndex(), result.Reason()};
    }
    return EvaluationDetail<std::string>{
        EvaluationReason::ErrorKind::kWrongType, default_value};
}

std::string ClientImpl::StringVariation(Context const& ctx,
                                        IClient::FlagKey const& key,
                                        std::string default_value) {
    auto result = Variation(ctx, key, default_value);
    if (result.IsString()) {
        return result;
    }
    return default_value;
}

EvaluationDetail<double> ClientImpl::DoubleVariationDetail(
    Context const& ctx,
    ClientImpl::FlagKey const& key,
    double default_value) {
    auto result = VariationDetail(ctx, key, default_value);
    if (result.Value().IsNumber()) {
        return EvaluationDetail<double>{result.Value(), result.VariationIndex(),
                                        result.Reason()};
    }
    return EvaluationDetail<double>{EvaluationReason::ErrorKind::kWrongType,
                                    default_value};
}

double ClientImpl::DoubleVariation(Context const& ctx,
                                   IClient::FlagKey const& key,
                                   double default_value) {
    auto result = Variation(ctx, key, default_value);
    if (result.IsNumber()) {
        return result;
    }
    return default_value;
}

EvaluationDetail<int> ClientImpl::IntVariationDetail(
    Context const& ctx,
    IClient::FlagKey const& key,
    int default_value) {
    auto result = VariationDetail(ctx, key, default_value);
    if (result.Value().IsNumber()) {
        return EvaluationDetail<int>{result.Value(), result.VariationIndex(),
                                     result.Reason()};
    }
    return EvaluationDetail<int>{EvaluationReason::ErrorKind::kWrongType,
                                 default_value};
}

int ClientImpl::IntVariation(Context const& ctx,
                             IClient::FlagKey const& key,
                             int default_value) {
    auto result = Variation(ctx, key, default_value);
    if (result.IsNumber()) {
        return result;
    }
    return default_value;
}

EvaluationDetail<Value> ClientImpl::JsonVariationDetail(
    Context const& ctx,
    IClient::FlagKey const& key,
    Value default_value) {
    return VariationDetail(ctx, key, default_value);
}

Value ClientImpl::JsonVariation(Context const& ctx,
                                IClient::FlagKey const& key,
                                Value default_value) {
    return Variation(ctx, key, default_value);
}

// data_sources::IDataSourceStatusProvider& ClientImpl::DataSourceStatus() {
//     return status_manager_;
// }
//
// flag_manager::IFlagNotifier& ClientImpl::FlagNotifier() {
//     return flag_manager_.Notifier();
// }

ClientImpl::~ClientImpl() {
    ioc_.stop();
    // TODO: Probably not the best.
    run_thread_.join();
}
}  // namespace launchdarkly::server_side
