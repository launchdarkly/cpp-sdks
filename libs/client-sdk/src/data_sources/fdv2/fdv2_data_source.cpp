#include "fdv2_data_source.hpp"

#include <launchdarkly/async/promise.hpp>

#include <boost/asio/post.hpp>

#include <algorithm>
#include <cassert>
#include <utility>
#include <variant>

namespace launchdarkly::client_side::data_sources {

namespace {

// Lets std::visit dispatch to a different lambda per variant alternative.
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// The distinctions the conditions act on, out of the whole result.
SourceSignal SignalFor(FDv2SourceResult const& result) {
    if (std::get_if<FDv2SourceResult::ChangeSet>(&result.value)) {
        return SourceSignal::kChangeSet;
    }
    if (std::get_if<FDv2SourceResult::Interrupted>(&result.value)) {
        return SourceSignal::kInterrupted;
    }
    return SourceSignal::kOther;
}

bool AllFromCache(
    std::vector<std::unique_ptr<IFDv2InitializerFactory>> const& factories) {
    return std::all_of(
        factories.begin(), factories.end(),
        [](auto const& factory) { return factory->IsFromCache(); });
}

}  // namespace

FDv2DataSource::FDv2DataSource(
    std::vector<std::unique_ptr<IFDv2InitializerFactory>> initializer_factories,
    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>>
        synchronizer_factories,
    std::unique_ptr<IFDv2ConditionFactory> fallback_condition_factory,
    std::unique_ptr<IFDv2ConditionFactory> recovery_condition_factory,
    boost::asio::any_io_executor executor,
    Context context,
    IDataSourceUpdateSink* sink,
    flag_manager::FlagStore const* store,
    DataSourceStatusManager* status_manager,
    Logger const& logger)
    : logger_(logger),
      executor_(std::move(executor)),
      initializer_factories_(std::move(initializer_factories)),
      fallback_condition_factory_(std::move(fallback_condition_factory)),
      recovery_condition_factory_(std::move(recovery_condition_factory)),
      context_(std::move(context)),
      cache_only_(!initializer_factories_.empty() &&
                  synchronizer_factories.empty() &&
                  AllFromCache(initializer_factories_)),
      sink_(sink),
      store_(store),
      status_manager_(status_manager),
      start_called_(false),
      last_logged_synchronizer_interrupted_(false),
      closed_(false),
      received_data_(false),
      initializer_index_(0),
      active_initializer_from_cache_(false),
      source_manager_(std::move(synchronizer_factories)),
      active_initializer_(nullptr),
      active_synchronizer_(nullptr),
      active_conditions_(nullptr) {}

FDv2DataSource::~FDv2DataSource() {
    Close();
}

void FDv2DataSource::Close() {
    std::lock_guard lock(mutex_);
    closed_ = true;
    if (active_initializer_) {
        active_initializer_->Close();
    }
    if (active_synchronizer_) {
        active_synchronizer_->Close();
    }
    if (active_conditions_) {
        active_conditions_->Close();
    }
}

std::optional<std::string> FDv2DataSource::EnvironmentId() const {
    std::lock_guard lock(mutex_);
    return environment_id_;
}

void FDv2DataSource::PublishState(DataSourceStatus::DataSourceState state) {
    {
        std::lock_guard lock(mutex_);
        if (closed_) {
            return;
        }
    }
    status_manager_->SetState(state);
}

void FDv2DataSource::PublishState(DataSourceStatus::DataSourceState state,
                                  DataSourceStatus::ErrorInfo::ErrorKind kind,
                                  std::string message) {
    {
        std::lock_guard lock(mutex_);
        if (closed_) {
            return;
        }
    }
    status_manager_->SetState(state, kind, std::move(message));
}

void FDv2DataSource::Start() {
    bool const already_called = start_called_.exchange(true);
    assert(!already_called && "Start() must be called at most once");

    PublishState(DataSourceStatus::DataSourceState::kInitializing);

    LD_LOG(logger_, LogLevel::kInfo) << "fdv2: starting";
    if (initializer_factories_.empty() &&
        source_manager_.SynchronizerCount() == 0) {
        // Nothing is configured to supply data, so an empty store is the
        // canonical state.
        PublishState(DataSourceStatus::DataSourceState::kValid);
        return;
    }

    // Evaluation can use cached flags as soon as Start() returns, the way it
    // could when the client loaded the cache in its constructor.
    RunCacheInitializers();

    boost::asio::post(executor_, [weak = weak_from_this()]() {
        if (auto self = weak.lock()) {
            self->RunNextInitializer();
        }
    });
}

void FDv2DataSource::RunCacheInitializers() {
    while (true) {
        std::unique_ptr<IFDv2Initializer> initializer;
        {
            std::lock_guard lock(mutex_);
            if (closed_ ||
                initializer_index_ >= initializer_factories_.size() ||
                !initializer_factories_[initializer_index_]->IsFromCache()) {
                return;
            }
            initializer = initializer_factories_[initializer_index_]->Build();
        }

        auto future = initializer->Run();
        if (!future.IsFinished()) {
            // Nothing here can wait on it, so the chain runs this initializer
            // instead. The index is left where it is.
            initializer->Close();
            return;
        }

        {
            std::lock_guard lock(mutex_);
            ++initializer_index_;
        }

        auto result = future.GetResult();
        if (result) {
            if (auto* change_set =
                    std::get_if<FDv2SourceResult::ChangeSet>(&result->value)) {
                LD_LOG(logger_, LogLevel::kInfo)
                    << "fdv2: applying cached data from "
                    << initializer->Identity();
                ApplyResult(std::move(*change_set),
                            std::move(result->environment_id),
                            /* from_cache= */ true);
            }
        }
        initializer->Close();
    }
}

void FDv2DataSource::ShutdownAsync(std::function<void()> completion) {
    // Report initializing so that a caller waiting on the next status change,
    // such as identify, sees the restart. This runs before Close(), which
    // stops any further transitions from this source.
    PublishState(DataSourceStatus::DataSourceState::kInitializing);
    Close();
    if (completion) {
        boost::asio::post(executor_, std::move(completion));
    }
}

void FDv2DataSource::RunNextInitializer() {
    bool exhausted = false;
    {
        std::lock_guard lock(mutex_);
        if (closed_) {
            return;
        }
        if (initializer_index_ >= initializer_factories_.size()) {
            exhausted = true;
        } else {
            auto& factory = initializer_factories_[initializer_index_++];
            active_initializer_from_cache_ = factory->IsFromCache();
            active_initializer_ = factory->Build();
            LD_LOG(logger_, LogLevel::kInfo) << "fdv2: starting initializer "
                                             << active_initializer_->Identity();
            active_initializer_->Run().Then(
                [weak = weak_from_this()](
                    FDv2SourceResult const& result) -> std::monostate {
                    if (auto self = weak.lock()) {
                        self->OnInitializerResult(result);
                    }
                    return {};
                },
                [executor = executor_](async::Continuation<void()> work) {
                    boost::asio::post(executor, std::move(work));
                });
        }
    }

    if (exhausted) {
        StartSynchronizers();
    }
}

void FDv2DataSource::OnInitializerResult(FDv2SourceResult result) {
    bool got_basis = false;
    bool got_shutdown = false;
    bool from_cache = false;
    {
        std::lock_guard lock(mutex_);
        from_cache = active_initializer_from_cache_;
    }

    std::visit(
        overloaded{
            [&](FDv2SourceResult::ChangeSet& cs) {
                bool const has_selector =
                    cs.change_set.selector.value.has_value();
                ApplyResult(std::move(cs), std::move(result.environment_id),
                            from_cache);
                if (has_selector) {
                    LD_LOG(logger_, LogLevel::kInfo)
                        << "fdv2: initializer succeeded";
                    got_basis = true;
                }
            },
            [&](FDv2SourceResult::Shutdown&) { got_shutdown = true; },
            [&](FDv2SourceResult::Interrupted const& iv) {
                LD_LOG(logger_, LogLevel::kWarn)
                    << "fdv2: initializer interrupted: " << iv.error.Message();
                PublishState(DataSourceStatus::DataSourceState::kInterrupted,
                             iv.error.Kind(), iv.error.Message());
            },
            [&](FDv2SourceResult::TerminalError const& te) {
                LD_LOG(logger_, LogLevel::kWarn)
                    << "fdv2: initializer terminal error: "
                    << te.error.Message();
                PublishState(DataSourceStatus::DataSourceState::kInterrupted,
                             te.error.Kind(), te.error.Message());
            },
            [&](FDv2SourceResult::Goodbye const&) {
                LD_LOG(logger_, LogLevel::kDebug)
                    << "fdv2: ignoring goodbye from initializer";
            },
        },
        result.value);

    {
        std::lock_guard lock(mutex_);
        active_initializer_.reset();
        if (closed_ || got_shutdown) {
            return;
        }
    }

    if (got_basis) {
        StartSynchronizers();
    } else {
        RunNextInitializer();
    }
}

void FDv2DataSource::StartSynchronizers() {
    bool exhausted = false;
    bool any_synchronizers_configured = false;
    {
        std::lock_guard lock(mutex_);
        if (closed_) {
            return;
        }
        active_synchronizer_ = source_manager_.NextSynchronizer();
        if (active_synchronizer_) {
            LD_LOG(logger_, LogLevel::kInfo)
                << "fdv2: starting synchronizer "
                << active_synchronizer_->Identity();
            last_logged_synchronizer_interrupted_.store(false);
            active_conditions_ = BuildActiveConditions();
        } else {
            exhausted = true;
            any_synchronizers_configured =
                source_manager_.SynchronizerCount() > 0;
        }
    }

    if (exhausted) {
        ReportExhausted(any_synchronizers_configured);
        return;
    }

    RunSynchronizerNext();
}

void FDv2DataSource::ReportExhausted(bool any_synchronizers_configured) {
    if (cache_only_) {
        // The cache is the only thing that could ever have supplied data, so
        // a miss is not a failure to initialize. It just means there are no
        // flags.
        PublishState(DataSourceStatus::DataSourceState::kValid);
        return;
    }

    bool received_data = false;
    {
        std::lock_guard lock(mutex_);
        received_data = received_data_;
    }
    if (!any_synchronizers_configured && received_data) {
        // The initializers supplied data and nothing is configured to keep it
        // current, which is a complete, successful run.
        return;
    }

    std::string const message =
        any_synchronizers_configured
            ? "all data source acquisition methods have been exhausted"
            : "all initializers exhausted and no synchronizers configured";
    LD_LOG(logger_, LogLevel::kWarn) << "fdv2: " << message;
    PublishState(DataSourceStatus::DataSourceState::kShutdown,
                 DataSourceStatus::ErrorInfo::ErrorKind::kUnknown, message);
}

void FDv2DataSource::RunSynchronizerNext() {
    std::lock_guard lock(mutex_);
    if (closed_ || !active_synchronizer_) {
        return;
    }
    auto next_future = active_synchronizer_->Next(store_->CurrentSelector());
    auto cond_cancel = std::make_shared<async::CancellationSource>();
    auto cond_future = active_conditions_->GetFuture(cond_cancel->GetToken());
    async::WhenAny(cond_future, next_future)
        .Then(
            [weak = weak_from_this(), cond_future, next_future,
             cond_cancel](std::size_t const& idx) -> std::monostate {
                cond_cancel->Cancel();
                auto self = weak.lock();
                if (!self) {
                    return {};
                }
                if (idx == 0) {
                    self->OnConditionFired(*cond_future.GetResult());
                } else {
                    self->OnSynchronizerResult(*next_future.GetResult());
                }
                return {};
            },
            [executor = executor_](async::Continuation<void()> work) {
                boost::asio::post(executor, std::move(work));
            });
}

void FDv2DataSource::OnConditionFired(IFDv2Condition::Type type) {
    if (type == IFDv2Condition::Type::kCancelled) {
        return;
    }
    {
        std::lock_guard lock(mutex_);
        if (closed_) {
            return;
        }
        // Destructors close the active synchronizer and conditions.
        active_synchronizer_.reset();
        active_conditions_.reset();
        if (type == IFDv2Condition::Type::kRecovery) {
            LD_LOG(logger_, LogLevel::kInfo) << "fdv2: recovery condition met";
            source_manager_.ResetSourceIndex();
        } else {
            LD_LOG(logger_, LogLevel::kInfo) << "fdv2: fallback condition met";
        }
    }
    StartSynchronizers();
}

std::unique_ptr<Conditions> FDv2DataSource::BuildActiveConditions() const {
    std::vector<std::unique_ptr<IFDv2Condition>> conditions;
    // With only one synchronizer available there's nothing to fall back to
    // or recover from, so leave the conditions empty.
    if (source_manager_.AvailableSynchronizerCount() == 1) {
        return std::make_unique<Conditions>(std::move(conditions));
    }
    if (fallback_condition_factory_) {
        conditions.push_back(fallback_condition_factory_->Build());
    }
    // The prime synchronizer has nothing more-preferred to recover to.
    if (!source_manager_.IsPrimeSynchronizer() && recovery_condition_factory_) {
        conditions.push_back(recovery_condition_factory_->Build());
    }
    return std::make_unique<Conditions>(std::move(conditions));
}

void FDv2DataSource::OnSynchronizerResult(FDv2SourceResult result) {
    {
        std::lock_guard lock(mutex_);
        if (closed_) {
            return;
        }
        if (active_conditions_) {
            active_conditions_->Inform(SignalFor(result));
        }
    }

    bool got_shutdown = false;
    bool advance = false;

    std::visit(
        overloaded{
            [&](FDv2SourceResult::ChangeSet& cs) {
                last_logged_synchronizer_interrupted_.store(false);
                ApplyResult(std::move(cs), std::move(result.environment_id),
                            /* from_cache= */ false);
            },
            [&](FDv2SourceResult::Shutdown&) { got_shutdown = true; },
            [&](FDv2SourceResult::Interrupted const& iv) {
                if (!last_logged_synchronizer_interrupted_.exchange(true)) {
                    LD_LOG(logger_, LogLevel::kInfo)
                        << "fdv2: synchronizer interrupted: "
                        << iv.error.Message();
                }
                PublishState(DataSourceStatus::DataSourceState::kInterrupted,
                             iv.error.Kind(), iv.error.Message());
            },
            [&](FDv2SourceResult::TerminalError const& te) {
                LD_LOG(logger_, LogLevel::kWarn)
                    << "fdv2: synchronizer terminal error: "
                    << te.error.Message();
                PublishState(DataSourceStatus::DataSourceState::kInterrupted,
                             te.error.Kind(), te.error.Message());
                advance = true;
            },
            [&](FDv2SourceResult::Goodbye const&) {
                // The synchronizer restarts its own connection.
            },
        },
        result.value);

    {
        std::lock_guard lock(mutex_);
        if (closed_ || got_shutdown) {
            active_synchronizer_.reset();
            active_conditions_.reset();
            return;
        }
        if (advance) {
            source_manager_.BlockCurrentSynchronizer();
            active_synchronizer_.reset();
            active_conditions_.reset();
        }
    }

    if (advance) {
        StartSynchronizers();
    } else {
        RunSynchronizerNext();
    }
}

void FDv2DataSource::ApplyResult(FDv2SourceResult::ChangeSet change_set,
                                 std::optional<std::string> environment_id,
                                 bool from_cache) {
    bool const carries_data =
        change_set.change_set.type != data_model::ChangeSetType::kNone;
    {
        std::lock_guard lock(mutex_);
        if (environment_id) {
            environment_id_ = std::move(environment_id);
        }
        received_data_ = received_data_ || carries_data;
    }
    sink_->Apply(context_, std::move(change_set.change_set), from_cache);
    PublishState(DataSourceStatus::DataSourceState::kValid);
}

}  // namespace launchdarkly::client_side::data_sources
