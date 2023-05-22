#pragma once

#include <chrono>
#include <functional>
#include <mutex>

#include "../data_sources/data_source_update_sink.hpp"
#include "context_index.hpp"
#include "flag_store.hpp"

#include <launchdarkly/context.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/persistence/persistence.hpp>

namespace launchdarkly::client_side::flag_manager {

std::string PersistenceEncodeKey(std::string const& input);

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

    void LoadCached(Context const& context);

   private:
    inline static std::string global_namespace_ = "LaunchDarkly";
    inline static std::string index_key_ = "ContextIndex";

    Logger& logger_;
    std::size_t max_cached_contexts_;

    IDataSourceUpdateSink& sink_;
    std::shared_ptr<IPersistence> persistence_;
    mutable std::recursive_mutex persistence_mutex_;
    FlagStore& flag_store_;

    std::string environment_namespace_;
    TimeStampsource time_stamper_;

    ContextIndex GetIndex();
    void StoreCache(std::string const& context_id);
};

}  // namespace launchdarkly::client_side::flag_manager
