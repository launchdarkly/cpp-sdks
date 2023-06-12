#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

#include "../data_sources/data_source_update_sink.hpp"
#include "context_index.hpp"

#include <launchdarkly/persistence/persistence.hpp>

namespace launchdarkly::client_side::flag_manager {

class FlagStore {
   public:
    void Init(std::unordered_map<std::string, ItemDescriptor> const& data);
    void Upsert(std::string const& key, ItemDescriptor item);

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

    std::unordered_map<std::string, std::shared_ptr<ItemDescriptor>> data_;
    mutable std::mutex data_mutex_;
};

}  // namespace launchdarkly::client_side::flag_manager
