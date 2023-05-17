#include "flag_persistence.hpp"
#include "../serialization/json_all_flags.hpp"

#include <launchdarkly/encoding/sha_256.hpp>

#include <utility>

namespace launchdarkly::client_side::flag_manager {

static std::string PersistenceEncodeKey(std::string const& input) {
    return encoding::Sha256String(input);
}

static std::string MakeEnvironment(std::string const& prefix,
                                   std::string const& sdk_key) {
    return prefix + "_" + PersistenceEncodeKey(sdk_key);
}

FlagPersistence::FlagPersistence(std::string const& sdk_key,
                                 IDataSourceUpdateSink* sink,
                                 FlagStore& flag_store,
                                 std::shared_ptr<IPersistence> persistence,
                                 Logger& logger,
                                 FlagPersistence::TimeStampsource time_stamper)
    : logger_(logger),
      sink_(sink),
      flag_store_(flag_store),
      persistence_(std::move(persistence)),
      environment_namespace_(MakeEnvironment(global_namespace_, sdk_key)),
      time_stamper_(time_stamper) {}

void FlagPersistence::Init(
    Context const& context,
    std::unordered_map<std::string, ItemDescriptor> data) {
    sink_->Init(context, std::move(data));
    StoreCache(PersistenceEncodeKey(context.canonical_key()));
}

void FlagPersistence::Upsert(Context const& context,
                             std::string key,
                             ItemDescriptor item) {
    sink_->Upsert(context, key, item);
    StoreCache(PersistenceEncodeKey(context.canonical_key()));
}

void FlagPersistence::LoadCached(Context const& context) {
    if (persistence_ && context.valid()) {
        std::lock_guard lock(persistence_mutex_);
        auto data =
            persistence_->Read(environment_namespace_,
                               PersistenceEncodeKey(context.canonical_key()));
        if (data) {
            boost::json::error_code error_code;
            auto parsed = boost::json::parse(*data, error_code);
            if (error_code) {
                LD_LOG(logger_, LogLevel::kError)
                    << "Failed to parse flag data from persistence.";
            } else {
                auto res = boost::json::value_to<tl::expected<
                    std::unordered_map<
                        std::string, launchdarkly::client_side::ItemDescriptor>,
                    JsonError>>(parsed);
                if (res) {
                    sink_->Init(context, *res);
                } else {
                    LD_LOG(logger_, LogLevel::kError)
                        << "Failed to parse flag data from persistence.";
                }
            }
        }
    }
}

void FlagPersistence::StoreCache(std::string const& context_id) {
    if (persistence_) {
        std::lock_guard lock(persistence_mutex_);
        auto index = GetIndex();
        index.Notice(context_id, time_stamper_());
        auto pruned = index.Prune(max_cached_contexts_);
        for (auto& id : pruned) {
            persistence_->RemoveValue(environment_namespace_, id);
        }
        persistence_->SetValue(
            environment_namespace_, index_key_,
            boost::json::serialize(boost::json::value_from(index)));

        persistence_->SetValue(environment_namespace_, context_id,
                               boost::json::serialize(boost::json::value_from(
                                   flag_store_.GetAll())));
    }
}

ContextIndex FlagPersistence::GetIndex() {
    if (persistence_) {
        std::lock_guard lock(persistence_mutex_);
        auto index_data = persistence_->Read(global_namespace_, index_key_);

        if (index_data) {
            boost::json::error_code error_code;
            auto parsed = boost::json::parse(*index_data, error_code);
            if (error_code) {
                LD_LOG(logger_, LogLevel::kError)
                    << "Failed to parse index data from persistence.";
            } else {
                return boost::json::value_to<ContextIndex>(std::move(parsed));
            }
        }
    }
    return ContextIndex();
}

}  // namespace launchdarkly::client_side::flag_manager
