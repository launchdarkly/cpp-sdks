#include "fdv2_data_system.hpp"

#include <launchdarkly/async/promise.hpp>
#include <launchdarkly/async/timer.hpp>

#include <boost/asio/post.hpp>

#include <cassert>
#include <utility>
#include <variant>

namespace launchdarkly::server_side::data_systems {

namespace {

// Lets std::visit dispatch to a different lambda per variant alternative.
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

}  // namespace

FDv2DataSystem::FDv2DataSystem(
    std::vector<std::unique_ptr<data_interfaces::IFDv2InitializerFactory>>
        initializer_factories,
    std::vector<std::unique_ptr<data_interfaces::IFDv2SynchronizerFactory>>
        synchronizer_factories,
    std::unique_ptr<data_interfaces::IFDv2ConditionFactory>
        fallback_condition_factory,
    std::unique_ptr<data_interfaces::IFDv2ConditionFactory>
        recovery_condition_factory,
    boost::asio::any_io_executor ioc,
    data_components::DataSourceStatusManager* status_manager,
    Logger const& logger)
    : logger_(logger),
      ioc_(std::move(ioc)),
      initializer_factories_(std::move(initializer_factories)),
      fallback_condition_factory_(std::move(fallback_condition_factory)),
      recovery_condition_factory_(std::move(recovery_condition_factory)),
      status_manager_(status_manager),
      store_(),
      change_notifier_(store_, store_),
      initialize_called_(false),
      last_logged_synchronizer_interrupted_(false),
      closed_(false),
      selector_(),
      initializer_index_(0),
      source_manager_(std::move(synchronizer_factories)),
      active_initializer_(nullptr),
      active_synchronizer_(nullptr),
      active_conditions_(nullptr) {}

FDv2DataSystem::~FDv2DataSystem() {
    Close();
}

void FDv2DataSystem::Close() {
    std::lock_guard<std::mutex> lock(mutex_);
    closed_ = true;
    fdv1_fallback_retry_cancel_.Cancel();
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

std::shared_ptr<data_model::FlagDescriptor> FDv2DataSystem::GetFlag(
    std::string const& key) const {
    return store_.GetFlag(key);
}

std::shared_ptr<data_model::SegmentDescriptor> FDv2DataSystem::GetSegment(
    std::string const& key) const {
    return store_.GetSegment(key);
}

std::unordered_map<std::string, std::shared_ptr<data_model::FlagDescriptor>>
FDv2DataSystem::AllFlags() const {
    return store_.AllFlags();
}

std::unordered_map<std::string, std::shared_ptr<data_model::SegmentDescriptor>>
FDv2DataSystem::AllSegments() const {
    return store_.AllSegments();
}

std::string const& FDv2DataSystem::Identity() const {
    static std::string const identity = "fdv2";
    return identity;
}

void FDv2DataSystem::Initialize() {
    bool const already_called = initialize_called_.exchange(true);
    assert(!already_called && "Initialize() must be called at most once");

    LD_LOG(logger_, LogLevel::kInfo) << Identity() << ": starting";
    if (initializer_factories_.empty() &&
        source_manager_.SynchronizerCount() == 0) {
        // Offline mode: empty store is the canonical state.
        status_manager_->SetState(DataSourceStatus::DataSourceState::kValid);
        return;
    }
    boost::asio::post(ioc_, [this]() { RunNextInitializer(); });
}

void FDv2DataSystem::RunNextInitializer() {
    bool exhausted = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_) {
            return;
        }
        if (initializer_index_ >= initializer_factories_.size()) {
            exhausted = true;
        } else {
            auto& factory = initializer_factories_[initializer_index_++];
            active_initializer_ = factory->Build();
            LD_LOG(logger_, LogLevel::kInfo)
                << Identity() << ": starting initializer "
                << active_initializer_->Identity();
            active_initializer_->Run().Then(
                [this](data_interfaces::FDv2SourceResult const& result)
                    -> std::monostate {
                    OnInitializerResult(result);
                    return {};
                },
                [ioc = ioc_](async::Continuation<void()> work) {
                    boost::asio::post(ioc, std::move(work));
                });
        }
    }

    if (exhausted) {
        StartSynchronizers();
    }
}

void FDv2DataSystem::OnInitializerResult(
    data_interfaces::FDv2SourceResult result) {
    using Result = data_interfaces::FDv2SourceResult;

    bool got_basis = false;
    bool got_shutdown = false;
    bool disconnected = false;

    std::visit(
        overloaded{
            [&](Result::ChangeSet& cs) {
                bool const has_selector =
                    cs.change_set.selector.value.has_value();
                ApplyChangeSet(std::move(cs.change_set));
                if (has_selector) {
                    LD_LOG(logger_, LogLevel::kInfo)
                        << Identity() << ": initializer succeeded";
                    got_basis = true;
                }
            },
            [&](Result::Shutdown&) { got_shutdown = true; },
            [&](Result::Interrupted const& iv) {
                LD_LOG(logger_, LogLevel::kWarn)
                    << Identity()
                    << ": initializer interrupted: " << iv.error.Message();
                status_manager_->SetState(
                    DataSourceStatus::DataSourceState::kInterrupted,
                    iv.error.Kind(), iv.error.Message());
            },
            [&](Result::TerminalError const& te) {
                LD_LOG(logger_, LogLevel::kWarn)
                    << Identity()
                    << ": initializer terminal error: " << te.error.Message();
                status_manager_->SetState(
                    DataSourceStatus::DataSourceState::kInterrupted,
                    te.error.Kind(), te.error.Message());
            },
            [&](Result::Goodbye const&) {
                LD_LOG(logger_, LogLevel::kDebug)
                    << Identity() << ": ignoring goodbye from initializer";
            },
        },
        result.value);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        active_initializer_.reset();
        if (closed_ || got_shutdown) {
            return;
        }
        if (result.fdv1_fallback) {
            source_manager_.SwitchToFDv1Fallback();
            if (source_manager_.AvailableSynchronizerCount() > 0) {
                LD_LOG(logger_, LogLevel::kInfo)
                    << Identity() << ": FDv1 fallback engaged";
                // No basis yet; reuse the flag to fall through to
                // StartSynchronizers so the FDv1 fallback synchronizer can
                // produce it.
                got_basis = true;
            } else {
                LD_LOG(logger_, LogLevel::kWarn)
                    << Identity()
                    << ": FDv1 fallback directive received; no FDv1 "
                       "fallback synchronizer configured";
                disconnected = true;
            }
            ScheduleFDv2RetryLocked(result.fdv1_fallback->ttl);
        }
    }

    if (disconnected) {
        status_manager_->SetState(
            DataSourceStatus::DataSourceState::kInterrupted,
            DataSourceStatus::ErrorInfo::ErrorKind::kUnknown,
            "FDv1 fallback directive received; no FDv1 fallback configured");
        return;
    }
    if (got_basis) {
        StartSynchronizers();
    } else {
        RunNextInitializer();
    }
}

void FDv2DataSystem::StartSynchronizers() {
    bool exhausted = false;
    // True if at least one synchronizer factory was configured at construction.
    bool any_synchronizers_configured = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_) {
            return;
        }
        active_synchronizer_ = source_manager_.NextSynchronizer();
        if (active_synchronizer_) {
            LD_LOG(logger_, LogLevel::kInfo)
                << Identity() << ": starting synchronizer "
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
        if (any_synchronizers_configured || !store_.Initialized()) {
            std::string const message =
                any_synchronizers_configured
                    ? "all data source acquisition methods have been exhausted"
                    : "all initializers exhausted and no synchronizers "
                      "configured";
            LD_LOG(logger_, LogLevel::kWarn) << Identity() << ": " << message;
            status_manager_->SetState(
                DataSourceStatus::DataSourceState::kOff,
                DataSourceStatus::ErrorInfo::ErrorKind::kUnknown, message);
        }
        return;
    }

    RunSynchronizerNext();
}

void FDv2DataSystem::RunSynchronizerNext() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_ || !active_synchronizer_) {
        return;
    }
    auto next_future = active_synchronizer_->Next(selector_);
    auto cond_future = active_conditions_->GetFuture();
    async::WhenAny(cond_future, next_future)
        .Then(
            [this, cond_future,
             next_future](std::size_t const& idx) -> std::monostate {
                if (idx == 0) {
                    OnConditionFired(*cond_future.GetResult());
                } else {
                    OnSynchronizerResult(*next_future.GetResult());
                }
                return {};
            },
            [ioc = ioc_](async::Continuation<void()> work) {
                boost::asio::post(ioc, std::move(work));
            });
}

void FDv2DataSystem::OnConditionFired(
    data_interfaces::IFDv2Condition::Type type) {
    using Type = data_interfaces::IFDv2Condition::Type;
    if (type == Type::kCancelled) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_) {
            return;
        }
        // Destructors close the active synchronizer and conditions.
        active_synchronizer_.reset();
        active_conditions_.reset();
        if (type == Type::kRecovery) {
            LD_LOG(logger_, LogLevel::kInfo)
                << Identity() << ": recovery condition met";
            source_manager_.ResetSourceIndex();
        } else {
            LD_LOG(logger_, LogLevel::kInfo)
                << Identity() << ": fallback condition met";
        }
    }
    StartSynchronizers();
}

std::unique_ptr<Conditions> FDv2DataSystem::BuildActiveConditions() const {
    std::vector<std::unique_ptr<data_interfaces::IFDv2Condition>> conditions;
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

void FDv2DataSystem::OnSynchronizerResult(
    data_interfaces::FDv2SourceResult result) {
    using Result = data_interfaces::FDv2SourceResult;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_) {
            return;
        }
        if (active_conditions_) {
            active_conditions_->Inform(result);
        }
    }

    bool got_shutdown = false;
    bool advance = false;
    bool disconnected = false;

    std::visit(
        overloaded{
            [&](Result::ChangeSet& cs) {
                last_logged_synchronizer_interrupted_.store(false);
                ApplyChangeSet(std::move(cs.change_set));
            },
            [&](Result::Shutdown&) { got_shutdown = true; },
            [&](Result::Interrupted const& iv) {
                if (!last_logged_synchronizer_interrupted_.exchange(true)) {
                    LD_LOG(logger_, LogLevel::kInfo)
                        << Identity()
                        << ": synchronizer interrupted: " << iv.error.Message();
                }
                status_manager_->SetState(
                    DataSourceStatus::DataSourceState::kInterrupted,
                    iv.error.Kind(), iv.error.Message());
            },
            [&](Result::TerminalError const& te) {
                LD_LOG(logger_, LogLevel::kWarn)
                    << Identity()
                    << ": synchronizer terminal error: " << te.error.Message();
                status_manager_->SetState(
                    DataSourceStatus::DataSourceState::kInterrupted,
                    te.error.Kind(), te.error.Message());
                advance = true;
            },
            [&](Result::Goodbye const&) {
                // The synchronizer handles this internally.
            },
        },
        result.value);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_ || got_shutdown) {
            active_synchronizer_.reset();
            active_conditions_.reset();
            return;
        }
        if (result.fdv1_fallback &&
            !source_manager_.IsCurrentSynchronizerFDv1Fallback()) {
            source_manager_.SwitchToFDv1Fallback();
            active_synchronizer_.reset();
            active_conditions_.reset();
            if (source_manager_.AvailableSynchronizerCount() > 0) {
                LD_LOG(logger_, LogLevel::kInfo)
                    << Identity() << ": FDv1 fallback engaged";
                advance = true;
            } else {
                LD_LOG(logger_, LogLevel::kWarn)
                    << Identity()
                    << ": FDv1 fallback directive received; no FDv1 "
                       "fallback synchronizer configured";
                disconnected = true;
            }
            ScheduleFDv2RetryLocked(result.fdv1_fallback->ttl);
        } else if (advance) {
            source_manager_.BlockCurrentSynchronizer();
            active_synchronizer_.reset();
            active_conditions_.reset();
        }
    }

    if (disconnected) {
        status_manager_->SetState(
            DataSourceStatus::DataSourceState::kInterrupted,
            DataSourceStatus::ErrorInfo::ErrorKind::kUnknown,
            "FDv1 fallback directive received; no FDv1 fallback configured");
        return;
    }
    if (advance) {
        StartSynchronizers();
    } else {
        RunSynchronizerNext();
    }
}

void FDv2DataSystem::ScheduleFDv2RetryLocked(std::chrono::seconds ttl) {
    if (ttl == std::chrono::seconds::zero()) {
        return;
    }
    LD_LOG(logger_, LogLevel::kInfo)
        << Identity() << ": FDv2 retry scheduled in " << ttl.count() << "s";
    async::Delay(ioc_, ttl, fdv1_fallback_retry_cancel_.GetToken())
        .Then(
            [this](bool fired) -> std::monostate {
                if (fired) {
                    OnFDv1RetryTimer();
                }
                return {};
            },
            [ioc = ioc_](async::Continuation<void()> work) {
                boost::asio::post(ioc, std::move(work));
            });
}

void FDv2DataSystem::OnFDv1RetryTimer() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_) {
            return;
        }
        LD_LOG(logger_, LogLevel::kInfo)
            << Identity() << ": FDv2 retry timer fired; re-engaging FDv2";
        source_manager_.SwitchBackToFDv2();
        if (active_synchronizer_) {
            active_synchronizer_->Close();
            active_synchronizer_.reset();
        }
        active_conditions_.reset();
    }
    StartSynchronizers();
}

void FDv2DataSystem::ApplyChangeSet(
    data_model::ChangeSet<data_interfaces::ChangeSetData> change_set) {
    if (change_set.selector.value.has_value()) {
        std::lock_guard<std::mutex> lock(mutex_);
        selector_ = change_set.selector;
    }
    change_notifier_.Apply(std::move(change_set));
    status_manager_->SetState(DataSourceStatus::DataSourceState::kValid);
}

bool FDv2DataSystem::Initialized() const {
    return store_.Initialized();
}

}  // namespace launchdarkly::server_side::data_systems
