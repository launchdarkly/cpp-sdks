#pragma once

#include <mutex>

#include "../data_sources/data_source_update_sink.hpp"
#include "context_index.hpp"
#include "flag_store.hpp"

#include <launchdarkly/client_side/persistence.hpp>
#include <launchdarkly/context.hpp>

namespace launchdarkly::client_side::flag_manager {

class FlagPersistence : public IDataSourceUpdateSink {
   public:
    FlagPersistence(IDataSourceUpdateSink* sink,
                    FlagStore& flag_store,
                    std::shared_ptr<IPersistence> persistence);

    void Init(Context const& context,
              std::unordered_map<std::string, ItemDescriptor> data) override;

    void Upsert(Context const& context,
                std::string key,
                ItemDescriptor item) override;

    void LoadCached(Context const& context);

   private:
    IDataSourceUpdateSink* sink_;
    std::shared_ptr<IPersistence> persistence_;
    mutable std::recursive_mutex persistence_mutex_;
    FlagStore& flag_store_;

    std::string environment_namespace_;
    // TODO: From config.
    std::size_t max_cached_contexts_ = 10;
    inline static std::string global_namespace_ = "LaunchDarkly";
    inline static std::string index_key_ = "ContextIndex";

    ContextIndex GetIndex();
    void StoreCache(std::string context_id);
};

}  // namespace launchdarkly::client_side::flag_manager