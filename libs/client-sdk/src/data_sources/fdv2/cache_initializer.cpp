#include "cache_initializer.hpp"

#include <utility>

namespace launchdarkly::client_side::data_sources {

static char const* const kIdentity = "FDv2 cache initializer";

FDv2CacheInitializer::FDv2CacheInitializer(flag_manager::FlagPersistence* cache,
                                           Context context,
                                           Logger const& logger)
    : cache_(cache), context_(std::move(context)), logger_(logger) {}

async::Future<FDv2SourceResult> FDv2CacheInitializer::Run() {
    auto data = cache_->ReadCached(context_);
    if (!data) {
        LD_LOG(logger_, LogLevel::kDebug)
            << kIdentity << ": no cached data for this context";
        // A miss leaves the data set unchanged so initialization can continue.
        return async::MakeFuture(FDv2SourceResult{FDv2SourceResult::ChangeSet{
            FlagChangeSet{data_model::ChangeSetType::kNone,
                          {},
                          data_model::Selector{}}}});
    }

    LD_LOG(logger_, LogLevel::kDebug)
        << kIdentity << ": loaded " << data->size()
        << " flags for this context";

    FlagChangeSetData changes;
    changes.reserve(data->size());
    for (auto& [key, item] : *data) {
        changes.push_back(FlagChange{key, std::move(item)});
    }

    return async::MakeFuture(FDv2SourceResult{FDv2SourceResult::ChangeSet{
        FlagChangeSet{data_model::ChangeSetType::kFull, std::move(changes),
                      data_model::Selector{}}}});
}

void FDv2CacheInitializer::Close() {
    // Run() completes on the calling thread, so there is nothing to cancel.
}

std::string const& FDv2CacheInitializer::Identity() const {
    static std::string const identity = kIdentity;
    return identity;
}

FDv2CacheInitializerFactory::FDv2CacheInitializerFactory(
    flag_manager::FlagPersistence* cache,
    Context context,
    Logger const& logger)
    : cache_(cache), context_(std::move(context)), logger_(logger) {}

std::unique_ptr<IFDv2Initializer> FDv2CacheInitializerFactory::Build() {
    return std::make_unique<FDv2CacheInitializer>(cache_, context_, logger_);
}

}  // namespace launchdarkly::client_side::data_sources
