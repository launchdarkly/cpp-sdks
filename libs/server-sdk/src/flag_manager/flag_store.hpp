#pragma once

#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/data_model/item_descriptor.hpp>
#include <mutex>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::flag_manager {

using ItemDescriptor = data_model::ItemDescriptor<data_model::Flag>;

class FlagStore {
   public:
    using Item = std::shared_ptr<ItemDescriptor>;
    void Init(std::unordered_map<std::string, ItemDescriptor> const& data);
    void Upsert(std::string const& key, ItemDescriptor item);

    /**
     * Attempts to get a flag by key from the current flags.
     *
     * @param flag_key The flag to get.
     * @return A shared_ptr to the value if present. A null shared_ptr if the
     * item is not present.
     */
    Item Get(std::string const& flag_key) const;

    /**
     * Gets all the current flags.
     *
     * @return All of the current flags.
     */
    std::unordered_map<std::string, Item> GetAll() const;

   private:
    void UpdateData(
        std::unordered_map<std::string, ItemDescriptor> const& data);

    std::unordered_map<std::string, Item> data_;
    mutable std::mutex data_mutex_;
};

}  // namespace launchdarkly::server_side::flag_manager
