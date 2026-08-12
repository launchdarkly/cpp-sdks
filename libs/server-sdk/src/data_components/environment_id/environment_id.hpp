#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <string_view>

namespace launchdarkly::server_side::data_components {

/**
 * @brief EnvironmentId holds the environment ID reported by LaunchDarkly.
 *
 * Data sources record the value from the X-LD-EnvID header of successful
 * streaming or polling responses; the client reads it when building hook
 * series contexts. Instances are shared between the data sources and the
 * client, and all methods are thread-safe.
 */
class EnvironmentId {
   public:
    /** Header carrying the environment ID on streaming/polling responses. */
    static constexpr char const* kHeader = "X-LD-EnvID";

    /**
     * @return The environment ID, if one has been reported.
     */
    [[nodiscard]] std::optional<std::string> Get() const;

    /**
     * Records an environment ID. Empty values are ignored.
     */
    void Set(std::string_view environment_id);

   private:
    mutable std::mutex mutex_;
    std::optional<std::string> environment_id_;
};

}  // namespace launchdarkly::server_side::data_components
