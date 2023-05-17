#include "flag_persistence.hpp"
#include "../encoding/sha_256.hpp"
#include "../serialization/json_all_flags.hpp"

#include <launchdarkly/serialization/json_errors.hpp>

namespace launchdarkly::client_side::flag_manager {

static std::string PersistenceEncodeKey(std::string const& input) {
    return encoding::Sha256String(input);
}

FlagPersistence::FlagPersistence(IDataSourceUpdateSink* sink,
                                 FlagStore& flag_store,
                                 std::shared_ptr<IPersistence> persistence)
    : sink_(sink), flag_store_(flag_store), persistence_(persistence) {}

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
                // TODO: Log?
            } else {
                auto res = boost::json::value_to<tl::expected<
                    std::unordered_map<
                        std::string, launchdarkly::client_side::ItemDescriptor>,
                    JsonError>>(parsed);
                if (res) {
                    sink_->Init(context, *res);
                } else {
                    // TODO: Log?
                }
            }
        }
    }
}

void FlagPersistence::StoreCache(std::string context_id) {
    if (persistence_) {
        std::lock_guard lock(persistence_mutex_);
        auto index = GetIndex();
        index.Notice(context_id);
        auto pruned = index.Prune(max_cached_contexts_);
        for (auto& id : pruned) {
            persistence_->RemoveValue(environment_namespace_, id);
        }
        persistence_->SetValue(
            global_namespace_, index_key_,
            boost::json::serialize(boost::json::value_from(index)));

        persistence_->SetValue(environment_namespace_, context_id,
                               boost::json::serialize(boost::json::value_from(
                                   flag_store_.GetAll())));
    }
}

persistence::ContextIndex FlagPersistence::GetIndex() {
    if (persistence_) {
        std::lock_guard lock(persistence_mutex_);
        auto index_data = persistence_->Read(global_namespace_, index_key_);

        if (index_data) {
            boost::json::error_code error_code;
            auto parsed = boost::json::parse(*index_data, error_code);
            if (error_code) {
                // TODO: Log?
            } else {
                return boost::json::value_to<persistence::ContextIndex>(
                    std::move(parsed));
            }
        }
    }
    return persistence::ContextIndex();
}

}  // namespace launchdarkly::client_side::flag_manager
