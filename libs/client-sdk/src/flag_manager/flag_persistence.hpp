#pragma once

#include <chrono>
#include <functional>
#include <mutex>
#include <optional>
#include <unordered_map>

#include "../data_sources/data_source_update_sink.hpp"
#include "context_index.hpp"
#include "flag_store.hpp"

#include <launchdarkly/context.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/persistence/persistence.hpp>

namespace launchdarkly::client_side::flag_manager {

std::string PersistenceEncodeKey(std::string const& input);

/**
 * Mirrors data source updates into the persistent store on their way to the
 * next sink, and reads them back when a context is loaded.
 *
 * Thread-safe. The methods that touch the store take persistence_mutex_,
 * which makes each stored index read-modify-write atomic. It does not span
 * the call to the downstream sink, so an update reaches the store and the
 * cache at slightly different times.
 */
class FlagPersistence : public IDataSourceUpdateSink {
   public:
    using TimeStampsource =
        std::function<std::chrono::time_point<std::chrono::system_clock>()>;

    FlagPersistence(
        std::string const& sdk_key,
        IDataSourceUpdateSink& sink,
        FlagStore& flag_store,
        std::shared_ptr<IPersistence> persistence,
        Logger& logger,
        std::size_t max_cached_contexts,
        TimeStampsource time_stamper = []() {
            return std::chrono::system_clock::now();
        });

    void Init(Context const& context,
              std::unordered_map<std::string, ItemDescriptor> data) override;

    void Upsert(Context const& context,
                std::string key,
                ItemDescriptor item) override;

    void Apply(Context const& context,
               FlagChangeSet change_set,
               bool from_cache) override;

    void LoadCached(Context const& context);

    /**
     * The flag data stored for the given context, or nullopt when nothing is
     * stored for it and when persistence is disabled. An empty map means an
     * environment with no flags was stored, which is distinct from nothing
     * being stored at all.
     */
    [[nodiscard]] std::optional<std::unordered_map<std::string, ItemDescriptor>>
    ReadCached(Context const& context);

    /**
     * When the service last confirmed the flag data for this context was
     * current, or nullopt if it never has.
     *
     * Keyed by the context's whole set of attributes rather than its key,
     * because changing an attribute can change how flags evaluate, and the
     * answer for the old attributes says nothing about the new ones.
     */
    [[nodiscard]] std::optional<
        std::chrono::time_point<std::chrono::system_clock>>
    FreshnessFor(Context const& context);

   private:
    inline static std::string global_namespace_ = "LaunchDarkly";
    inline static std::string index_key_ = "ContextIndex";
    inline static std::string freshness_key_ = "ContextFreshness";

    Logger& logger_;
    std::size_t max_cached_contexts_;

    IDataSourceUpdateSink& sink_;
    FlagStore& flag_store_;

    // Serializes the read-modify-write of the stored index and flag data, so
    // that two contexts being cached at once cannot lose an index entry.
    mutable std::recursive_mutex persistence_mutex_;
    std::shared_ptr<IPersistence> persistence_;

    std::string environment_namespace_;
    TimeStampsource time_stamper_;

    void StoreCache(std::string const& context_id);

    // Records that the service confirmed this context's data is current, as
    // of now.
    void RecordFreshness(Context const& context);

    // Must be called with persistence_mutex_ held.
    ContextIndex ReadIndexAt(std::string const& key);
};

}  // namespace launchdarkly::client_side::flag_manager
