#include "client_impl.hpp"

#include "all_flags_state/all_flags_state_builder.hpp"
#include "data_systems/background_sync/background_sync_system.hpp"
#include "data_systems/lazy_load/lazy_load_system.hpp"
#include "data_systems/offline.hpp"
#include "evaluation/evaluation_stack.hpp"

#include "data_interfaces/system/idata_system.hpp"

#include <launchdarkly/config/shared/built/http_properties.hpp>
#include <launchdarkly/encoding/sha_256.hpp>
#include <launchdarkly/events/asio_event_processor.hpp>
#include <launchdarkly/events/data/common_events.hpp>
#include <launchdarkly/logging/console_backend.hpp>
#include <launchdarkly/logging/null_logger.hpp>

#include <launchdarkly/server_side/config/builders/all_builders.hpp>
#include <launchdarkly/server_side/config/built/all_built.hpp>

#include <chrono>
#include <optional>
#include <utility>

namespace launchdarkly::server_side {

using EventProcessor = events::AsioEventProcessor<config::builders::SDK>;

// The ASIO implementation assumes that the io_context will be run from a
// single thread, and applies several optimisations based on this
// assumption.
auto const kAsioConcurrencyHint = 1;

// Client's destructor attempts to gracefully shut down the datasource
// connection in this amount of time.
auto const kDataSourceShutdownWait = std::chrono::milliseconds(100);

static std::unique_ptr<data_interfaces::IDataSystem> MakeDataSystem(
    config::built::HttpProperties const& http_properties,
    Config const& config,
    boost::asio::any_io_executor const& executor,
    data_components::DataSourceStatusManager& status_manager,
    Logger& logger) {
    if (config.DataSystemConfig().disabled) {
        return std::make_unique<data_systems::OfflineSystem>(status_manager);
    }

    auto const builder =
        config::builders::HttpPropertiesBuilder(http_properties);

    auto data_source_properties = builder.Build();

    return std::visit(
        [&](auto&& arg) -> std::unique_ptr<data_interfaces::IDataSystem> {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T,
                                         config::built::BackgroundSyncConfig>) {
                return std::make_unique<data_systems::BackgroundSync>(
                    config.ServiceEndpoints(), arg, data_source_properties,
                    executor, status_manager, logger);
            } else if constexpr (std::is_same_v<
                                     T, config::built::LazyLoadConfig>) {
                return std::make_unique<data_systems::LazyLoad>(logger, arg,
                                                                status_manager);
            }
        },
        config.DataSystemConfig().system_);
}

static Logger MakeLogger(config::built::Logging const& config) {
    if (config.disable_logging) {
        return {std::make_shared<logging::NullLoggerBackend>()};
    }
    if (config.backend) {
        return {config.backend};
    }
    return {
        std::make_shared<logging::ConsoleBackend>(config.level, config.tag)};
}

std::unique_ptr<EventProcessor> MakeEventProcessor(
    Config const& config,
    boost::asio::any_io_executor const& exec,
    config::built::HttpProperties const& http_properties,
    Logger& logger) {
    if (config.Events().Enabled()) {
        return std::make_unique<EventProcessor>(exec, config.ServiceEndpoints(),
                                                config.Events(),
                                                http_properties, logger);
    }
    return nullptr;
}

/**
 * Returns true if the flag pointer is valid and the underlying item is
 * present.
 */
bool IsFlagPresent(
    std::shared_ptr<data_model::FlagDescriptor> const& flag_desc);

ClientImpl::ClientImpl(Config config, std::string const& version)
    : config_(config),
      http_properties_(
          config::builders::HttpPropertiesBuilder(config.HttpProperties())
              .Header("user-agent", "CPPServer/" + version)
              .Header("authorization", config.SdkKey())
              .Header("x-launchdarkly-tags", config.ApplicationTag())
              .Build()),
      logger_(MakeLogger(config.Logging())),
      ioc_(kAsioConcurrencyHint),
      work_(boost::asio::make_work_guard(ioc_)),
      status_manager_(),
      data_system_(MakeDataSystem(http_properties_,
                                  config_,
                                  ioc_.get_executor(),
                                  status_manager_,
                                  logger_)),
      event_processor_(MakeEventProcessor(config,
                                          ioc_.get_executor(),
                                          http_properties_,
                                          logger_)),
      evaluator_(logger_, *data_system_),
      events_default_(event_processor_.get(), EventFactory::WithoutReasons()),
      events_with_reasons_(event_processor_.get(),
                           EventFactory::WithReasons()) {
    LD_LOG(logger_, LogLevel::kDebug)
        << "data system: " << data_system_->Identity();
    if (auto custom_ca = http_properties_.Tls().CustomCAFile()) {
        LD_LOG(logger_, LogLevel::kInfo)
            << "TLS peer verification configured with custom CA file: "
            << *custom_ca;
    }
    if (http_properties_.Tls().PeerVerifyMode() ==
        launchdarkly::config::shared::built::TlsOptions::VerifyMode::
            kVerifyNone) {
        LD_LOG(logger_, LogLevel::kInfo) << "TLS peer verification disabled";
    }

    run_thread_ = std::move(std::thread([&]() { ioc_.run(); }));
}

void ClientImpl::Identify(Context context) {
    events_default_.Send([&](EventFactory const& factory) {
        return factory.Identify(std::move(context));
    });
}

std::future<bool> ClientImpl::StartAsync() {
    auto pr = std::make_shared<std::promise<bool>>();
    auto fut = pr->get_future();

    status_manager_.OnDataSourceStatusChangeEx([this, pr](auto _) {
        if (data_system_->Initialized()) {
            pr->set_value(true);
            return true; /* delete this change listener since the
                            desired state was reached */
        }
        return false; /* keep the change listener */
    });

    data_system_->Initialize();

    return fut;
}

bool ClientImpl::Initialized() const {
    return data_system_->Initialized();
}

AllFlagsState ClientImpl::AllFlagsState(Context const& context,
                                        AllFlagsState::Options options) {
    std::unordered_map<Client::FlagKey, Value> result;

    if (!Initialized()) {
        LD_LOG(logger_, LogLevel::kWarn)
            << "AllFlagsState() called before client has finished "
               "initializing. Data source not available. Returning empty state";

        return {};
    }

    AllFlagsStateBuilder builder{options};

    EventScope no_events;

    auto all_flags = data_system_->AllFlags();

    // Because evaluating the flags may access many segments, tell the data
    // system to fetch them all at once up-front. This may be a no-op
    // depending on the data system (e.g. if the segments are all already in
    // memory.)
    auto _ = data_system_->AllSegments();

    for (auto const& [k, v] : all_flags) {
        if (!v || !v->item) {
            continue;
        }

        auto const& flag = *(v->item);

        if (IsSet(options, AllFlagsState::Options::ClientSideOnly) &&
            !flag.clientSideAvailability.usingEnvironmentId) {
            continue;
        }

        EvaluationDetail<Value> detail =
            evaluator_.Evaluate(flag, context, no_events);

        bool in_experiment = flag.IsExperimentationEnabled(detail.Reason());
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
    events_default_.Send([&](EventFactory const& factory) {
        return factory.Custom(ctx, std::move(event_name), std::move(data),
                              metric_value);
    });
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
    if (event_processor_) {
        event_processor_->FlushAsync();
    }
}

void ClientImpl::LogVariationCall(std::string const& key,
                                  bool flag_present) const {
    if (Initialized()) {
        if (!flag_present) {
            LD_LOG(logger_, LogLevel::kInfo) << "Unknown feature flag " << key
                                             << "; returning default value";
        }
    } else {
        if (flag_present) {
            LD_LOG(logger_, LogLevel::kInfo)
                << "LaunchDarkly client has not yet been initialized; using "
                   "last "
                   "known flag rules from data store";
        } else {
            LD_LOG(logger_, LogLevel::kInfo)
                << "LaunchDarkly client has not yet been initialized; "
                   "returning default value";
        }
    }
}

Value ClientImpl::Variation(Context const& ctx,
                            enum Value::Type value_type,
                            IClient::FlagKey const& key,
                            Value const& default_value) {
    auto result = *VariationInternal(ctx, key, default_value, events_default_);
    if (result.Type() != value_type) {
        return default_value;
    }
    return result;
}

EvaluationDetail<Value> ClientImpl::VariationInternal(
    Context const& context,
    IClient::FlagKey const& key,
    Value const& default_value,
    EventScope const& event_scope) {
    if (auto error = PreEvaluationChecks(context)) {
        return PostEvaluation(key, context, default_value, *error, event_scope,
                              std::nullopt);
    }

    auto flag_rule = data_system_->GetFlag(key);

    bool flag_present = IsFlagPresent(flag_rule);

    LogVariationCall(key, flag_present);

    if (!flag_present) {
        return PostEvaluation(key, context, default_value,
                              EvaluationReason::ErrorKind::kFlagNotFound,
                              event_scope, std::nullopt);
    }

    EvaluationDetail<Value> result =
        evaluator_.Evaluate(*flag_rule->item, context, event_scope);
    return PostEvaluation(key, context, default_value, result, event_scope,
                          flag_rule.get()->item);
}

std::optional<enum EvaluationReason::ErrorKind> ClientImpl::PreEvaluationChecks(
    Context const& context) const {
    if (!Initialized()) {
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
        error_or_detail,
    EventScope const& event_scope,
    std::optional<data_model::Flag> const& flag) {
    return std::visit(
        [&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            // VARIANT: ErrorKind
            if constexpr (std::is_same_v<T, enum EvaluationReason::ErrorKind>) {
                auto detail = EvaluationDetail<Value>{arg, default_value};

                event_scope.Send([&](EventFactory const& factory) {
                    return factory.UnknownFlag(key, context, detail,
                                               default_value);
                });

                return detail;
            }
            // VARIANT: EvaluationDetail
            else if constexpr (std::is_same_v<T, EvaluationDetail<Value>>) {
                auto detail = EvaluationDetail<Value>{
                    (!arg.VariationIndex() ? default_value : arg.Value()),
                    arg.VariationIndex(), arg.Reason()};

                event_scope.Send([&](EventFactory const& factory) {
                    return factory.Eval(key, context, flag, detail,
                                        default_value, std::nullopt);
                });

                return detail;
            }
        },
        std::move(error_or_detail));
}

bool IsFlagPresent(
    std::shared_ptr<data_model::FlagDescriptor> const& flag_desc) {
    return flag_desc && flag_desc->item;
}

EvaluationDetail<bool> ClientImpl::BoolVariationDetail(
    Context const& ctx,
    IClient::FlagKey const& key,
    bool default_value) {
    return VariationDetail<bool>(ctx, Value::Type::kBool, key, default_value);
}

bool ClientImpl::BoolVariation(Context const& ctx,
                               IClient::FlagKey const& key,
                               bool default_value) {
    return Variation(ctx, Value::Type::kBool, key, default_value);
}

EvaluationDetail<std::string> ClientImpl::StringVariationDetail(
    Context const& ctx,
    ClientImpl::FlagKey const& key,
    std::string default_value) {
    return VariationDetail<std::string>(ctx, Value::Type::kString, key,
                                        default_value);
}

std::string ClientImpl::StringVariation(Context const& ctx,
                                        IClient::FlagKey const& key,
                                        std::string default_value) {
    return Variation(ctx, Value::Type::kString, key, default_value);
}

EvaluationDetail<double> ClientImpl::DoubleVariationDetail(
    Context const& ctx,
    ClientImpl::FlagKey const& key,
    double default_value) {
    return VariationDetail<double>(ctx, Value::Type::kNumber, key,
                                   default_value);
}

double ClientImpl::DoubleVariation(Context const& ctx,
                                   IClient::FlagKey const& key,
                                   double default_value) {
    return Variation(ctx, Value::Type::kNumber, key, default_value);
}

EvaluationDetail<int> ClientImpl::IntVariationDetail(
    Context const& ctx,
    IClient::FlagKey const& key,
    int default_value) {
    return VariationDetail<int>(ctx, Value::Type::kNumber, key, default_value);
}

int ClientImpl::IntVariation(Context const& ctx,
                             IClient::FlagKey const& key,
                             int default_value) {
    return Variation(ctx, Value::Type::kNumber, key, default_value);
}

EvaluationDetail<Value> ClientImpl::JsonVariationDetail(
    Context const& ctx,
    IClient::FlagKey const& key,
    Value default_value) {
    return VariationInternal(ctx, key, default_value, events_with_reasons_);
}

Value ClientImpl::JsonVariation(Context const& ctx,
                                IClient::FlagKey const& key,
                                Value default_value) {
    return *VariationInternal(ctx, key, default_value, events_default_);
}

IDataSourceStatusProvider& ClientImpl::DataSourceStatus() {
    return status_manager_;
}

ClientImpl::~ClientImpl() {
    ioc_.stop();
    // TODO(SC-219101)
    run_thread_.join();
}
}  // namespace launchdarkly::server_side
