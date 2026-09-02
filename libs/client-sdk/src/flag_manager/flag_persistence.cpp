#include "flag_persistence.hpp"

#include <launchdarkly/encoding/base_64.hpp>
#include <launchdarkly/encoding/sha_256.hpp>

#include <launchdarkly/detail/serialization/json_primitives.hpp>
#include <launchdarkly/serialization/json_context.hpp>
#include <launchdarkly/serialization/json_evaluation_result.hpp>
#include <launchdarkly/serialization/json_item_descriptor.hpp>

#include <utility>

namespace launchdarkly::client_side::flag_manager {

std::string PersistenceEncodeKey(std::string const& input) {
    std::array<unsigned char, SHA256_DIGEST_LENGTH> bytes =
        encoding::Sha256String(input);
    std::string byte_str(reinterpret_cast<char*>(bytes.data()), bytes.size());
    return encoding::Base64UrlEncode(byte_str);
}

static std::string MakeEnvironment(std::string const& prefix,
                                   std::string const& sdk_key) {
    return prefix + "_" + PersistenceEncodeKey(sdk_key);
}

FlagPersistence::FlagPersistence(std::string const& sdk_key,
                                 IDataSourceUpdateSink& sink,
                                 FlagStore& flag_store,
                                 std::shared_ptr<IPersistence> persistence,
                                 Logger& logger,
                                 std::size_t max_cached_contexts,
                                 FlagPersistence::TimeStampsource time_stamper)
    : logger_(logger),
      sink_(sink),
      flag_store_(flag_store),
      persistence_(std::move(persistence)),
      environment_namespace_(MakeEnvironment(global_namespace_, sdk_key)),
      time_stamper_(time_stamper),
      max_cached_contexts_(max_cached_contexts) {}

void FlagPersistence::Init(
    Context const& context,
    std::unordered_map<std::string, ItemDescriptor> data) {
    sink_.Init(context, std::move(data));
    StoreCache(PersistenceEncodeKey(context.CanonicalKey()));
}

void FlagPersistence::Upsert(Context const& context,
                             std::string key,
                             ItemDescriptor item) {
    sink_.Upsert(context, key, item);
    StoreCache(PersistenceEncodeKey(context.CanonicalKey()));
}

void FlagPersistence::Apply(Context const& context,
                            FlagChangeSet change_set,
                            bool from_cache) {
    bool const changed_data =
        change_set.type != data_model::ChangeSetType::kNone;
    sink_.Apply(context, std::move(change_set), from_cache);
    if (from_cache) {
        // Writing cached data back to the cache would be a no-op, and it was
        // never confirmed current by the service.
        return;
    }
    // Both a payload and a confirmation that nothing changed mean the cache
    // is up to date as of now.
    RecordFreshness(context);
    if (changed_data) {
        StoreCache(PersistenceEncodeKey(context.CanonicalKey()));
    }
}

std::optional<std::unordered_map<std::string, ItemDescriptor>>
FlagPersistence::ReadCached(Context const& context) {
    if (!persistence_ || !context.Valid()) {
        return std::nullopt;
    }

    std::lock_guard lock(persistence_mutex_);
    auto data = persistence_->Read(
        environment_namespace_, PersistenceEncodeKey(context.CanonicalKey()));

    if (!data) {
        return std::nullopt;
    }

    boost::system::error_code error_code;
    auto parsed = boost::json::parse(*data, error_code);
    if (error_code) {
        LD_LOG(logger_, LogLevel::kError)
            << "Failed to parse flag data from persistence: "
            << error_code.message();
        return std::nullopt;
    }

    auto res = boost::json::value_to<tl::expected<
        std::optional<std::unordered_map<std::string, ItemDescriptor>>,
        JsonError>>(parsed);
    if (!res) {
        LD_LOG(logger_, LogLevel::kError)
            << "Failed to parse flag data from persistence";
        return std::nullopt;
    }

    // If the map was null or omitted, treat it like an empty data set.
    return res.value().value_or(
        std::unordered_map<std::string, ItemDescriptor>{});
}

void FlagPersistence::LoadCached(Context const& context) {
    if (auto data = ReadCached(context)) {
        sink_.Init(context, std::move(*data));
    }
}

// Identifies a context by everything it carries, not just its key. Changing
// an attribute can change how flags evaluate.
static std::string FreshnessId(Context const& context) {
    return PersistenceEncodeKey(
        boost::json::serialize(boost::json::value_from(context)));
}

void FlagPersistence::RecordFreshness(Context const& context) {
    if (!persistence_ || !context.Valid()) {
        return;
    }

    std::lock_guard lock(persistence_mutex_);
    auto index = ReadIndexAt(freshness_key_);
    index.Notice(FreshnessId(context), time_stamper_());
    index.Prune(max_cached_contexts_);
    persistence_->Set(environment_namespace_, freshness_key_,
                      boost::json::serialize(boost::json::value_from(index)));
}

std::optional<std::chrono::time_point<std::chrono::system_clock>>
FlagPersistence::FreshnessFor(Context const& context) {
    if (!persistence_ || !context.Valid()) {
        return std::nullopt;
    }

    std::lock_guard lock(persistence_mutex_);
    return ReadIndexAt(freshness_key_).TimestampFor(FreshnessId(context));
}

void FlagPersistence::StoreCache(std::string const& context_id) {
    if (!persistence_) {
        return;
    }

    std::lock_guard lock(persistence_mutex_);
    auto index = ReadIndexAt(index_key_);
    index.Notice(context_id, time_stamper_());
    auto pruned = index.Prune(max_cached_contexts_);
    for (auto& id : pruned) {
        persistence_->Remove(environment_namespace_, id);
    }
    persistence_->Set(environment_namespace_, index_key_,
                      boost::json::serialize(boost::json::value_from(index)));

    boost::json::value v = boost::json::value_from(flag_store_.GetAll());

    persistence_->Set(environment_namespace_, context_id,
                      boost::json::serialize(v));
}

ContextIndex FlagPersistence::ReadIndexAt(std::string const& key) {
    if (persistence_) {
        auto index_data = persistence_->Read(environment_namespace_, key);

        if (index_data) {
            boost::system::error_code error_code;
            auto parsed = boost::json::parse(*index_data, error_code);
            if (error_code) {
                LD_LOG(logger_, LogLevel::kError)
                    << "Failed to parse index data from persistence: "
                    << error_code.message() << " " << *index_data;
            } else {
                return boost::json::value_to<ContextIndex>(std::move(parsed));
            }
        }
    }
    return ContextIndex();
}

}  // namespace launchdarkly::client_side::flag_manager
