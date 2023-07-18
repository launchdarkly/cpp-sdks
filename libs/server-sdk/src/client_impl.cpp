
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
      event_processor_(nullptr) {
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

    return fut;
}

std::future<bool> ClientImpl::StartAsync() {
    return StartAsyncInternal(IsInitializedSuccessfully);
}

bool ClientImpl::Initialized() const {
    return IsInitializedSuccessfully(status_manager_.Status().State());
}

std::unordered_map<Client::FlagKey, Value> ClientImpl::AllFlagsState() const {
    std::unordered_map<Client::FlagKey, Value> result;
    // TODO: implement all flags state (and update signature).
    //    for (auto& [key, descriptor] : memory_store_.AllFlags()) {
    //        if (descriptor->item) {
    //            result.try_emplace(key, descriptor->item->Value());
    //        }
    //    }
    return result;
}

void ClientImpl::TrackInternal(Context const& ctx,
                               std::string event_name,
                               std::optional<Value> data,
                               std::optional<double> metric_value) {
    event_processor_->SendAsync(events::TrackEventParams{
        std::chrono::system_clock::now(), std::move(event_name),
        ctx.KindsToKeys(), std::move(data), metric_value});
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
    //    auto desc = memory_store_.GetFlag(key);
    //
    //    events::FeatureEventParams event = {
    //        std::chrono::system_clock::now(),
    //        key,
    //        ctx,
    //        default_value,
    //        default_value,
    //        std::nullopt,
    //        std::nullopt,
    //        std::nullopt,
    //        false,
    //        std::nullopt,
    //    };
    //
    //    if (!desc || !desc->item) {
    //        if (!Initialized()) {
    //            LD_LOG(logger_, LogLevel::kWarn)
    //                << "LaunchDarkly client has not yet been initialized.
    //                "
    //                   "Returning default value";
    //
    //            auto error_reason =
    //                EvaluationReason(EvaluationReason::ErrorKind::kClientNotReady);
    //            event.reason = error_reason;
    //
    //            event_processor_->SendAsync(std::move(event));
    //            return EvaluationDetail<T>(default_value, std::nullopt,
    //                                       std::move(error_reason));
    //        }
    //
    //        LD_LOG(logger_, LogLevel::kInfo)
    //            << "Unknown feature flag " << key << "; returning default
    //            value";
    //
    //        auto error_reason =
    //            EvaluationReason(EvaluationReason::ErrorKind::kFlagNotFound);
    //        event.reason = error_reason;
    //
    //        event_processor_->SendAsync(std::move(event));
    //        return EvaluationDetail<T>(default_value, std::nullopt,
    //                                   std::move(error_reason));
    //
    //    } else if (!Initialized()) {
    //        LD_LOG(logger_, LogLevel::kInfo)
    //            << "LaunchDarkly client has not yet been initialized. "
    //               "Returning cached value";
    //    }
    //
    //    assert(desc->item);
    //
    //    auto const& flag = *(desc->item);
    //
    //    // TODO: evaluation
    //
    //    if (check_type && default_value.Type() != Value::Type::kNull &&
    //        detail.Value().Type() != default_value.Type()) {
    //        auto error_reason =
    //            EvaluationReason(EvaluationReason::ErrorKind::kWrongType);
    //        event.reason = error_reason;
    //
    //        event_processor_->SendAsync(std::move(event));
    //        return EvaluationDetail<T>(default_value, std::nullopt,
    //        error_reason);
    //    }
    //
    //    event.value = detail.Value();
    //    event.variation = detail.VariationIndex();
    //
    //    if (detailed || flag.TrackReason()) {
    //        event.reason = detail.Reason();
    //    }
    //
    //    event.version = flag.FlagVersion().value_or(flag.Version());
    //    event.require_full_event = flag.TrackEvents();
    //    if (auto date = flag.DebugEventsUntilDate()) {
    //        event.debug_events_until_date = events::Date{*date};
    //    }
    //
    //    event_processor_->SendAsync(std::move(event));
    //
    //    return EvaluationDetail<T>(detail.Value(),
    //    detail.VariationIndex(),
    //                               detail.Reason());

    return EvaluationDetail<T>(EvaluationReason::ErrorKind::kFlagNotFound,
                               Value::Null());
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

}  // namespace launchdarkly::server_side
