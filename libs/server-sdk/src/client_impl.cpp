
#include <chrono>

#include <optional>
#include <utility>

#include "client_impl.hpp"

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

bool IsExperimentationEnabled(data_model::Flag const& flag,
                              std::optional<EvaluationReason> const& reason);

AllFlagsStateOptions operator&(AllFlagsStateOptions lhs,
                               AllFlagsStateOptions rhs);

bool IsSet(AllFlagsStateOptions options, AllFlagsStateOptions flag);

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
      evaluator_(logger_, memory_store_) {
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
    event_processor_->SendAsync(events::IdentifyEventParams{
        std::chrono::system_clock::now(), std::move(context)});
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

FeatureFlagsState ClientImpl::AllFlagsState(Context const& context,
                                            enum AllFlagsStateOptions options) {
    std::unordered_map<Client::FlagKey, Value> result;

    if (config_.Offline()) {
        LD_LOG(logger_, LogLevel::kWarn)
            << "AllFlagsState() called, but client is in offline mode. "
               "Returning empty state";
        return FeatureFlagsState{};
    }

    if (!Initialized()) {
        LD_LOG(logger_, LogLevel::kWarn)
            << "AllFlagsState() called before client has finished "
               "initializing! Feature store unavailable - returning empty "
               "state";
        return FeatureFlagsState{};
    }

    std::unordered_map<std::string, FeatureFlagsState::FlagState> flags_state;
    std::unordered_map<std::string, Value> evaluations;

    for (auto [key, flag_desc] : memory_store_.AllFlags()) {
        if (!flag_desc || !flag_desc->item) {
            continue;
        }
        auto const& flag = (*flag_desc->item);
        if (IsSet(options, AllFlagsStateOptions::ClientSideOnly) &&
            !flag.clientSideAvailability.usingEnvironmentId) {
            continue;
        }

        auto detail = evaluator_.Evaluate(flag, context);

        bool require_experiment_data =
            IsExperimentationEnabled(flag, detail.Reason());
        bool track_events = flag.trackEvents || require_experiment_data;
        bool track_reason = require_experiment_data;
        bool currently_debugging = false;
        if (flag.debugEventsUntilDate) {
            auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();
            currently_debugging = *flag.debugEventsUntilDate > now;
        }

        bool omit_details =
            (IsSet(options, AllFlagsStateOptions::DetailsOnlyForTrackedFlags) &&
             !(track_events || track_reason || currently_debugging));

        std::optional<EvaluationReason> reason =
            (!IsSet(options, AllFlagsStateOptions::IncludeReasons) &&
             !track_reason)
                ? std::nullopt
                : detail.Reason();

        if (omit_details) {
            reason = std::nullopt;
            /* version = std::nullopt; */
        }

        evaluations.emplace(key, detail.Value());
        flags_state.emplace(
            key, FeatureFlagsState::FlagState{
                     flag.version, detail.VariationIndex(), reason,
                     track_events, track_reason, flag.debugEventsUntilDate});
    }

    return FeatureFlagsState{std::move(evaluations), std::move(flags_state)};
}

bool IsExperimentationEnabled(data_model::Flag const& flag,
                              std::optional<EvaluationReason> const& reason) {
    if (!reason) {
        return false;
    }
    if (reason->InExperiment()) {
        return true;
    }
    switch (reason->Kind()) {
        case EvaluationReason::Kind::kFallthrough:
            return flag.trackEventsFallthrough;
        case EvaluationReason::Kind::kRuleMatch:
            if (!reason->RuleIndex() ||
                reason->RuleIndex() >= flag.rules.size()) {
                return false;
            }
            return flag.rules.at(*reason->RuleIndex()).trackEvents;
        default:
            return false;
    }
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

template <typename T>
EvaluationDetail<T> ClientImpl::VariationInternal(Context const& ctx,
                                                  FlagKey const& key,
                                                  Value default_value,
                                                  bool check_type,
                                                  bool detailed) {
    events::FeatureEventParams event = {
        std::chrono::system_clock::now(),
        key,
        ctx,
        default_value,
        default_value,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        false,
        std::nullopt,
    };

    auto desc = memory_store_.GetFlag(key);

    if (!desc || !desc->item) {
        if (!Initialized()) {
            LD_LOG(logger_, LogLevel::kWarn)
                << "LaunchDarkly client has not yet been initialized. "
                   "Returning default value";

            auto error_reason =
                EvaluationReason(EvaluationReason::ErrorKind::kClientNotReady);

            if (detailed) {
                event.reason = error_reason;
            }

            event_processor_->SendAsync(std::move(event));
            return EvaluationDetail<T>(std::move(default_value), std::nullopt,
                                       std::move(error_reason));
        }

        LD_LOG(logger_, LogLevel::kInfo)
            << "Unknown feature flag " << key << "; returning default value";

        auto error_reason =
            EvaluationReason(EvaluationReason::ErrorKind::kFlagNotFound);

        if (detailed) {
            event.reason = error_reason;
        }
        event_processor_->SendAsync(std::move(event));
        return EvaluationDetail<T>(std::move(default_value), std::nullopt,
                                   std::move(error_reason));

    } else if (!Initialized()) {
        LD_LOG(logger_, LogLevel::kInfo)
            << "LaunchDarkly client has not yet been initialized. "
               "Returning cached value";
    }

    assert(desc->item);

    auto const& flag = *(desc->item);

    EvaluationDetail<Value> const detail = evaluator_.Evaluate(flag, ctx);

    if (check_type && default_value.Type() != Value::Type::kNull &&
        detail.Value().Type() != default_value.Type()) {
        auto error_reason =
            EvaluationReason(EvaluationReason::ErrorKind::kWrongType);

        if (detailed) {
            event.reason = error_reason;
        }
        event_processor_->SendAsync(std::move(event));
        return EvaluationDetail<T>(std::move(default_value), std::nullopt,
                                   error_reason);
    }

    event.value = detail.Value();
    event.variation = detail.VariationIndex();
    event.version = flag.Version();

    if (detailed) {
        event.reason = detail.Reason();
    }

    if (flag.debugEventsUntilDate) {
        event.debug_events_until_date =
            events::Date{std::chrono::system_clock::time_point{
                std::chrono::milliseconds{*flag.debugEventsUntilDate}}};
    }

    bool track_fallthrough =
        flag.trackEventsFallthrough &&
        detail.ReasonKindIs(EvaluationReason::Kind::kFallthrough);

    bool track_rule_match =
        detail.ReasonKindIs(EvaluationReason::Kind::kRuleMatch);

    if (track_rule_match) {
        auto const& rule_index = detail.Reason()->RuleIndex();
        assert(rule_index &&
               "evaluation algorithm must produce a rule index in the case of "
               "rule "
               "match");

        assert(*rule_index < flag.rules.size() &&
               "evaluation algorithm must produce a valid rule index in the "
               "case of "
               "rule match");

        track_rule_match = flag.rules.at(*rule_index).trackEvents;
    }

    event.require_full_event =
        flag.trackEvents || track_fallthrough || track_rule_match;
    event_processor_->SendAsync(std::move(event));
    return EvaluationDetail<T>(detail.Value(), detail.VariationIndex(),
                               detail.Reason());
}

EvaluationDetail<bool> ClientImpl::BoolVariationDetail(
    Context const& ctx,
    IClient::FlagKey const& key,
    bool default_value) {
    return VariationInternal<bool>(ctx, key, default_value, true, true);
}

bool ClientImpl::BoolVariation(Context const& ctx,
                               IClient::FlagKey const& key,
                               bool default_value) {
    return *VariationInternal<bool>(ctx, key, default_value, true, false);
}

EvaluationDetail<std::string> ClientImpl::StringVariationDetail(
    Context const& ctx,
    ClientImpl::FlagKey const& key,
    std::string default_value) {
    return VariationInternal<std::string>(ctx, key, std::move(default_value),
                                          true, true);
}

std::string ClientImpl::StringVariation(Context const& ctx,
                                        IClient::FlagKey const& key,
                                        std::string default_value) {
    return *VariationInternal<std::string>(ctx, key, std::move(default_value),
                                           true, false);
}

EvaluationDetail<double> ClientImpl::DoubleVariationDetail(
    Context const& ctx,
    ClientImpl::FlagKey const& key,
    double default_value) {
    return VariationInternal<double>(ctx, key, default_value, true, true);
}

double ClientImpl::DoubleVariation(Context const& ctx,
                                   IClient::FlagKey const& key,
                                   double default_value) {
    return *VariationInternal<double>(ctx, key, default_value, true, false);
}

EvaluationDetail<int> ClientImpl::IntVariationDetail(
    Context const& ctx,
    IClient::FlagKey const& key,
    int default_value) {
    return VariationInternal<int>(ctx, key, default_value, true, true);
}

int ClientImpl::IntVariation(Context const& ctx,
                             IClient::FlagKey const& key,
                             int default_value) {
    return *VariationInternal<int>(ctx, key, default_value, true, false);
}

EvaluationDetail<Value> ClientImpl::JsonVariationDetail(
    Context const& ctx,
    IClient::FlagKey const& key,
    Value default_value) {
    return VariationInternal<Value>(ctx, key, std::move(default_value), false,
                                    true);
}

Value ClientImpl::JsonVariation(Context const& ctx,
                                IClient::FlagKey const& key,
                                Value default_value) {
    return *VariationInternal<Value>(ctx, key, std::move(default_value), false,
                                     false);
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

AllFlagsStateOptions operator&(AllFlagsStateOptions lhs,
                               AllFlagsStateOptions rhs) {
    return static_cast<AllFlagsStateOptions>(
        static_cast<std::underlying_type_t<AllFlagsStateOptions>>(lhs) &
        static_cast<std::underlying_type_t<AllFlagsStateOptions>>(rhs));
}

bool IsSet(AllFlagsStateOptions options, AllFlagsStateOptions flag) {
    return (options & flag) == flag;
}

}  // namespace launchdarkly::server_side
