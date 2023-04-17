#pragma once

#include <mutex>

#include "launchdarkly/client_side/data_source_update_sink.hpp"

namespace launchdarkly::client_side::flag_manager::detail {

class FlagManager {
   public:
    void init(const std::unordered_map<std::string, ItemDescriptor>& data);
    void upsert(const std::string& key, ItemDescriptor item);

    /**
     * Attempts to get a flag by key from the current flags.
     *
     * @param flag_key The flag to get.
     * @return A shared_ptr to the value if present. A null shared_ptr if the
     * item is not present.
     */
    std::shared_ptr<ItemDescriptor> get(const std::string& flag_key) const;

    /**
     * Gets all the current flags.
     *
     * @return All of the current flags.
     */
    std::unordered_map<std::string, std::shared_ptr<ItemDescriptor>> get_all()
        const;

   private:
    std::unordered_map<std::string, std::shared_ptr<ItemDescriptor>> data_;
    mutable std::mutex data_mutex_;
};

}  // namespace launchdarkly::client_side::flag_manager::detail
