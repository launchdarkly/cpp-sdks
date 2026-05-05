#include "fdv2_data_system.hpp"

#include <launchdarkly/async/promise.hpp>

#include <boost/asio/post.hpp>

#include <chrono>
#include <utility>
#include <variant>

namespace launchdarkly::server_side::data_systems {

namespace {

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// Default until fallback/recovery is implemented.
constexpr std::chrono::hours kSynchronizerNextTimeout{24};

}  // namespace

FDv2DataSystem::FDv2DataSystem(
    std::vector<std::unique_ptr<data_interfaces::IFDv2InitializerFactory>>
        initializer_factories,
    std::vector<std::unique_ptr<data_interfaces::IFDv2SynchronizerFactory>>
        synchronizer_factories,
    boost::asio::any_io_executor ioc,
    data_components::DataSourceStatusManager* status_manager,
    Logger const& logger)
    : logger_(logger),
      ioc_(std::move(ioc)),
      initializer_factories_(std::move(initializer_factories)),
      synchronizer_factories_(std::move(synchronizer_factories)),
      status_manager_(status_manager),
      store_(),
      change_notifier_(store_, store_),
      closed_(false),
      selector_(),
      initializer_index_(0),
      synchronizer_index_(0),
      active_initializer_(nullptr),
      active_synchronizer_(nullptr) {}

FDv2DataSystem::~FDv2DataSystem() {
    Close();
}

void FDv2DataSystem::Close() {
    std::lock_guard<std::mutex> lock(mutex_);
    closed_ = true;
    if (active_initializer_) {
        active_initializer_->Close();
    }
    if (active_synchronizer_) {
        active_synchronizer_->Close();
    }
    status_manager_->SetState(DataSourceStatus::DataSourceState::kOff);
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
    LD_LOG(logger_, LogLevel::kInfo) << Identity() << ": starting";
    if (initializer_factories_.empty() && synchronizer_factories_.empty()) {
        // Offline mode: empty store is the canonical state.
        status_manager_->SetState(DataSourceStatus::DataSourceState::kValid);
        return;
    }
    boost::asio::post(ioc_, [this]() { RunNextInitializer(); });
}

void FDv2DataSystem::RunNextInitializer() {
    auto future = async::MakeFuture(data_interfaces::FDv2SourceResult{
        data_interfaces::FDv2SourceResult::Shutdown{}});
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
            future = active_initializer_->Run();
        }
    }

    if (exhausted) {
        StartSynchronizers();
        return;
    }

    std::move(future).Then(
        [this](
            data_interfaces::FDv2SourceResult const& result) -> std::monostate {
            OnInitializerResult(result);
            return {};
        },
        async::kInlineExecutor);
}

void FDv2DataSystem::OnInitializerResult(
    data_interfaces::FDv2SourceResult result) {
    using Result = data_interfaces::FDv2SourceResult;

    bool got_basis = false;
    bool got_shutdown = false;

    std::visit(
        overloaded{
            [&](Result::ChangeSet& cs) {
                bool const has_selector =
                    cs.change_set.selector.value.has_value();
                ApplyChangeSet(std::move(cs.change_set));
                if (has_selector) {
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
                LD_LOG(logger_, LogLevel::kWarn)
                    << Identity()
                    << ": initializer received unexpected goodbye";
            },
            [&](Result::Timeout const&) {
                LD_LOG(logger_, LogLevel::kWarn)
                    << Identity() << ": initializer timed out (unexpected)";
            },
        },
        result.value);

    {
        std::lock_guard<std::mutex> lock(mutex_);
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

void FDv2DataSystem::StartSynchronizers() {
    bool exhausted = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_) {
            return;
        }
        if (synchronizer_index_ >= synchronizer_factories_.size()) {
            exhausted = true;
        } else {
            auto& factory = synchronizer_factories_[synchronizer_index_++];
            active_synchronizer_ = factory->Build();
        }
    }

    if (exhausted) {
        LD_LOG(logger_, LogLevel::kWarn)
            << Identity() << ": no synchronizers available";
        return;
    }

    RunSynchronizerNext();
}

void FDv2DataSystem::RunSynchronizerNext() {
    auto future = async::MakeFuture(data_interfaces::FDv2SourceResult{
        data_interfaces::FDv2SourceResult::Shutdown{}});
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_ || !active_synchronizer_) {
            return;
        }
        future =
            active_synchronizer_->Next(kSynchronizerNextTimeout, selector_);
    }

    std::move(future).Then(
        [this](
            data_interfaces::FDv2SourceResult const& result) -> std::monostate {
            OnSynchronizerResult(result);
            return {};
        },
        async::kInlineExecutor);
}

void FDv2DataSystem::OnSynchronizerResult(
    data_interfaces::FDv2SourceResult result) {
    using Result = data_interfaces::FDv2SourceResult;

    bool got_shutdown = false;
    bool advance = false;

    std::visit(
        overloaded{
            [&](Result::ChangeSet& cs) {
                ApplyChangeSet(std::move(cs.change_set));
            },
            [&](Result::Shutdown&) { got_shutdown = true; },
            [&](Result::Interrupted const& iv) {
                LD_LOG(logger_, LogLevel::kWarn)
                    << Identity()
                    << ": synchronizer interrupted: " << iv.error.Message();
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
            [&](Result::Goodbye const& gb) {
                LD_LOG(logger_, LogLevel::kInfo)
                    << Identity() << ": synchronizer goodbye"
                    << (gb.reason ? ": " + *gb.reason : "");
                advance = true;
            },
            [&](Result::Timeout const&) {
                LD_LOG(logger_, LogLevel::kDebug)
                    << Identity() << ": synchronizer timed out; retrying";
            },
        },
        result.value);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_ || got_shutdown) {
            active_synchronizer_.reset();
            return;
        }
        if (advance) {
            active_synchronizer_.reset();
        }
    }

    if (advance) {
        StartSynchronizers();
    } else {
        RunSynchronizerNext();
    }
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
