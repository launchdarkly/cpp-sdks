#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

#include "../data_sources/data_source_update_sink.hpp"

#include <launchdarkly/client_side/persistence.hpp>

namespace launchdarkly::client_side::flag_manager {

class FlagManager {
   public:
    FlagManager(std::shared_ptr<IPersistence> persistence,
                std::string environment_namespace);
    void Init(std::unordered_map<std::string, ItemDescriptor> const& data);
    void Upsert(std::string const& key, ItemDescriptor item);

    void LoadCache(Context const& context);

    /**
     * Attempts to get a flag by key from the current flags.
     *
     * @param flag_key The flag to get.
     * @return A shared_ptr to the value if present. A null shared_ptr if the
     * item is not present.
     */
    std::shared_ptr<ItemDescriptor> Get(std::string const& flag_key) const;

    /**
     * Gets all the current flags.
     *
     * @return All of the current flags.
     */
    std::unordered_map<std::string, std::shared_ptr<ItemDescriptor>> GetAll()
        const;

   private:
    void UpdateData(
        std::unordered_map<std::string, ItemDescriptor> const& data);

    void StoreCache(std::string context_id);

    std::unordered_map<std::string, std::shared_ptr<ItemDescriptor>> data_;
    mutable std::mutex data_mutex_;
    mutable std::recursive_mutex persistence_mutex_;
    std::shared_ptr<IPersistence> persistence_;
    std::string environment_namespace_;

    // TODO: From config.
    std::size_t max_cached_contexts_ = 10;

    inline static std::string global_namespace_ = "LaunchDarkly";
    inline static std::string index_key_ = "ContextIndex";
    persistence::ContextIndex GetIndex();
};

}  // namespace launchdarkly::client_side::flag_manager
